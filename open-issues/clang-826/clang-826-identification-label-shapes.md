# CLANG-826: `identification_label` accepts only arrays of strings or pointers, and combining a pointer-array label loses its names

**Status:** Open -- filed 2026-10-23; not fixed (recorded by instruction)
**Kind:** defect (a conforming label is rejected or misread)
**Upstream:** `--` (contracts are not upstream in Clang)
**Mirror:** gnu_gcc's libstdc++ `<contracts>` spells the concept the same
way (`group_names[0]` convertible to `const char*`); gnu_gcc GCC-899
**Affects:** `std::contracts::labels::identification_label` and the
combined-label group merge (`__group_count`, `__group_max_len`,
`__gn_append`, `__combined_identification_label`) in
`libcxx/include/contracts`; Sema's `extractGroupNames` in
`clang/lib/Sema/SemaContract.cpp`
**Workaround:** spell `group_names` as an array of character arrays
(`static constexpr char group_names[][N] = {...}`), the shape
`assertion_group_label` uses
**Measured:** 2026-10-23, this branch's install of `a16e84eff550`, by hand
(no lit run)
**Watch tests:**
[`OpenBugs/identification-label-string-view-range.cpp`](../../clang/test/Contracts/OpenBugs/identification-label-string-view-range.cpp)
(the concept),
[`OpenBugs/identification-label-string-view-groups.cpp`](../../clang/test/Contracts/OpenBugs/identification-label-string-view-groups.cpp)
(Sema reads no groups),
[`OpenBugs/identification-label-pointer-array-combine.cpp`](../../clang/test/Contracts/OpenBugs/identification-label-pointer-array-combine.cpp)
(the combined-label merge)

## What the wording requires

D3400R5 [support.contract.control.ident]:

```c++
template<class T>
  concept identification_label =
    assertion_control_object<T> &&
    requires (const T t) {
      requires is_const_v<decltype(t.group_names)>;
      { t.group_names } -> ranges::range;
      { *(ranges::begin(t.group_names)) } -> convertible_to<string_view>;
    };
```

and [support.contract.control.combine] makes a combined label's
`group_names` the sorted, unique union of its constituents' group names.

## Symptoms

1. **The concept is narrower.**  libc++ requires
   `__t.group_names[0]` convertible to `const char*`.  A label whose
   `group_names` is `static constexpr std::array<std::string_view, 2>` or
   `static constexpr std::string_view group_names[]` does not satisfy
   `identification_label`:

   ```
   note: because '__is_convertible(decltype(__t.group_names[0]), const char *)' evaluated to false
   ```

   Any non-indexable range (a constexpr view, a class with `begin`/`end`
   only) is rejected for the same reason.

2. **Sema reads no groups from such a label.**  `extractGroupNames` reads
   `group_names[i]` converted to `const char*` for the
   `sizeof(group_names) / sizeof(group_names[0])` elements, so a
   `std::string_view` element is skipped.  `pre<sv_array>(x > 0)` with
   `-fcontract-evaluation-semantic=enforce
   -fcontract-group-evaluation-semantic=safety:observe` is enforced, not
   observed: the program aborts (exit 134) on the violation.  With
   `std::array<const char *, 1>` in place of `std::array<std::string_view, 1>`
   the same program observes the violation (exit 0).

3. **The combined-label merge assumes character arrays.**  libc++ accepts
   an array of pointers to strings as `group_names`
   (`p3400-group-names-pointer-array.cpp` passes), but `__group_max_len` is
   `sizeof(_Tp::group_names[0])` -- 8 for `const char *` -- and
   `__gn_append` copies that many characters from each name:
   * a name shorter than 8 characters (`{"a"}`, or `{"safe"}`) makes the
     combination fail constant evaluation:

     ```
     error: constexpr variable 'both' must be initialized by a constant expression
     contracts:527:27: note: read of dereferenced one-past-the-end pointer is not allowed in a constant expression
     ```

   * a longer name (`{"safetycritical"}`) combined with `"zz"group` is cut
     to 8 characters with no terminator, and the combined label is in no
     group: neither `safetycritical:observe` nor `safetycr:observe`
     matches, and the contract is enforced (exit 134).  The same labels
     spelled as character arrays (`char group_names[][15]`) combine and
     match (exit 0).

## Fix

Specify the concept as the wording does (a range whose elements convert to
`string_view`; including `<string_view>` and enough of `<ranges>` or a
hand-rolled `begin`/`end` probe), make Sema iterate the range and convert
each element to `string_view` (or evaluate `data()`/`size()`), and make the
merge copy each name by its length rather than by `sizeof` the element.
Mirror in libstdc++ and gnu_gcc's group reader.
