# CLANG-809: `-fsanitize=alignment` does not check an access or a placement new through a pointer formed from a character array

**Status:** Open here; open upstream ([#177682](https://github.com/llvm/llvm-project/issues/177682),
the load and store regression; the placement-new and `&a[o]` forms are
not mentioned there).  The native P3100 checks on this branch: the
placement-new check (`ub:basic.align.object.alignment.nostore`) reports
every form; the `.store` check of an access misses the same forms
(CLANG-810, ours, fixed here).  Whether to fix the stock sanitizer here too
was DECISIONS.md E127 (E84 / E91 precedent): deferred by the user
2026-10-19 (E127 (a)); this report, the `verify.sh` case and the watch
test stay, the upstream comment stays owed
**Component:** clang:codegen
**Upstream Link:** [#177682](https://github.com/llvm/llvm-project/issues/177682)
(open, filed 2026-01-23 by the author of #152575; not ours) -- comment
pending (the placement-new form and the `&a[o]` forms)
**Affects:** measured 2026-10-18 at -O0 and -O2 -- 18.1.0, 19.1.0, 20.1.0,
21.1.0, 22.1.6, the stock trunk nightly (`33891afe2ebb`) and this branch's
install (`81526623b898`):

| form | 18.1.0..21.1.0 | 22.1.6, trunk, this branch | gcc |
|------|----------------|----------------------------|-----|
| `*(int *)(buf + o)`, load | 1 | 0 | 1 |
| `*(int *)(buf + o) = 1`, store | 1 | 0 | 1 |
| `*(int *)(lb + o)`, a local array | 1 | 0 | 1 |
| `((P *)(buf + o))->a`, member read | 2 (member access, load) | 1 (member access) | 1 |
| `*(int *)&buf[o]` | 0 | 0 | 1 |
| `new (s + o) S()`, `new (s + o) T` | 1 | 0 | 0 (GCC-877) |
| `new (&s[o]) S()` | 0 | 0 | 0 (GCC-877) |
| controls: `*(int *)(c + o)`, `c` a `char *` parameter; `void *m = s + o; new (m) S()` | 1 | 1 | 1 / 0 |

**D4277R1 category:** `ub:basic.align.object.alignment` (UBSan's
`-fsanitize=alignment`)

## Reproducer

[`clang-809-ubsan-alignment-char-array.cpp`](clang-809-ubsan-alignment-char-array.cpp),
at `-std=c++17 -fsanitize=alignment`, run: it prints the number of UBSan
reports of each form at `o = 1` (every form is misaligned).  Clang 22.1.6
and trunk print `load=0 store=0 load_elem=0 pnew=0 pnew_elem=0 load_ptr=1
pnew_ptr=1`; 18.1.0..21.1.0 print `load=1 store=1 load_elem=0 pnew=1
pnew_elem=0 load_ptr=1 pnew_ptr=1`.

## Cause

One cause for every form.  `EmitTypeCheck` takes the alignment it checks
from the alignment CodeGen derived for the address (`Alignment`, the
lvalue's or the allocation's), not from the type: a derived alignment of 1
means no check at all (`AlignVal > 1`).  `EmitPointerWithAlignment` keeps
the alignment it derives from a declaration (`AlignmentSource::Decl`)
through an explicit cast to a pointer type, instead of taking the cast-to
type's (`if (InnerBaseInfo.getAlignmentSource() != AlignmentSource::Decl)`
in the `CK_BitCast` case), and `EmitCXXNewExpr` does the same for a
placement argument.  So a pointer formed from a character array keeps the
array element's alignment, 1, and its use is never checked:

* `&buf[o]` has always been derived from the declaration of `buf`, so
  `*(int *)&buf[o]` and `new (&s[o]) S()` have never been checked.
* `buf + o` was given the pointee type's natural alignment until
  [#152575](https://github.com/llvm/llvm-project/pull/152575) ("Preserve
  alignment information for pointer arithmetics", 2025-08-25, Clang 22),
  which derives it from the operand as for `&buf[o]`; since then
  `*(int *)(buf + o)` and `new (s + o) S()` are not checked either.

Through a pointer variable the derived alignment is the pointee type's,
so the controls are checked.  The derived alignment is right for code
generation (the loads are emitted `align 1`); the sanitizer needs the
alignment the type requires.  #177682 asks to restore the stricter check.

## Upstream search (2026-10-18)

GitHub issue search on llvm/llvm-project including closed issues.
Queries: `fsanitize=alignment misaligned char array`; `alignment sanitizer
placement new misaligned`; `152575`; `ubsan alignment pointer arithmetic
regression`; `fsanitize=alignment array cast`; `alignment sanitizer
misaligned array element cast`.  #177682 (open) is the load/store
regression after #152575, with reproducers using a constant subscript; it
does not mention the placement-new form or that `&a[o]` was never checked.

## Draft upstream comment (on #177682; the user posts it)

    The same derived alignment also hides a misaligned placement new from
    -fsanitize=alignment, and the &a[i] spelling was never checked:

        struct alignas(4) S {};
        void f(int o) {                  // o == 1
          alignas(S) char s[sizeof(S) + 1];
          new (s + o) S();               // 21.1: reported; 22.1 and trunk: not
          new (&s[o]) S();               // never reported (18.1 .. trunk)
        }
        alignas(8) char buf[16];
        int g(int o) { return *(int *)&buf[o]; }   // never reported

    Through a pointer variable (void *m = s + o; new (m) S();) all of them
    are.  EmitCXXNewExpr keeps the Decl-derived alignment of the placement
    argument, as EmitPointerWithAlignment does through the cast.

## Found by

M-Docs (part 2) slice A (the placement-new form), and M93-GCC's
measurement for CLANG-808 (the loads and stores); measured and filed by
M94-Clang.
