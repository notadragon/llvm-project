# CLANG-811: a range-based for over a null array reported three times at -O2

**Status:** Accepted by the user 2026-10-19 (DECISIONS.md E128 (a)):
the count depends on the optimization level, pinned at 2 (-O0) and 3 (-O2)
**Kind:** divergence (gnu_gcc reports twice at -O0 and -O2)
**Upstream:** `--` (the P3100 check is ours)
**Measured:** 2026-10-18, this branch's install of `81526623b898`, gnu_gcc's
install of the `15aa33328ff` tree, `ub:expr.unary.dereference.nullptr`
(the group or `.indirection`) configured
**Pinned by:** `clang/test/Contracts/Runnable/p3100-null-binding-location.cpp`
(`rfor`, GRP0/GRP2, IND0/IND2)

## Symptom

```c++
int rfor(int (*ap)[2]) {           // ap null
  int n = 0;
  for (int &x : *ap) n += &x == nullptr;
  return n;
}
```

Clang reports this twice at -O0 and three times at -O2 (an `int (*)[3]`:
twice and four times); gnu_gcc twice at every level.  The range binding
`auto &&__range = *ap` is reported once; then each element's binding,
`int &x = *__begin`, is checked on `__begin`: the first element's address
is null (reported), the next ones are `ap + 1`, `ap + 2` (not null) at -O0.
At -O1 and above the optimizer folds `__begin + k == null` into
`__range == null` (an inbounds address is null only if its base is), so
every element's check reports.  Each report names the loop's line.

## Analysis

The source performs one indirection through the null pointer (`*ap`) and
then indirections through `__begin` -- null for the first element, the
UB arithmetic `null + k` for the rest.  DECISIONS.md E99 / E103 (a) put
the null check of an indirection through explicit pointer arithmetic on
the innermost pointer at every level, but `*__begin` is an indirection
through a variable, not through arithmetic, so neither settles it; E110
(a) is about a leaf, not a count; E120 is about one null dereference
reported by two checks.  Hence E128, answered (a): the divergence is
accepted.

## Found by

M93-GCC's ad-hoc measurement for CLANG-807; judged by M94-Clang.
