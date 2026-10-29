# CLANG-664: the noexcept operator ignores an initializer list's array filler

**Kind:** defect (wrong `noexcept` result; wrong code in a contract predicate)
**Status:** Fixed here (M17-Clang, 2026-10-09) -- found 2026-10-09, M17-Clang, moving contract predicates
onto `Sema::canThrow` (CLANG-587)
**Component:** clang / Sema (`Sema::canThrow`, `SemaExceptionSpec.cpp`)
**Upstream Link:** None found (searched 2026-10-09: GitHub issue search on
llvm/llvm-project for "noexcept array filler", "noexcept InitListExpr",
"canThrow InitListExpr", "noexcept aggregate array constructor"; nothing
relevant)
**Affects:** measured 2026-10-09 with `-std=c++20 -fsyntax-only` -- Clang
18.1.0, 22.1.6, trunk `33891afe2ebb` and this branch fail the three
filler cases of the reproducer
**Workaround:** none in general; naming every element explicitly avoids it
**GCC:** wrong the same way (15.3.0 and our trunk-based install)

## What is wrong

[`clang-664-noexcept-array-filler.cpp`](clang-664-noexcept-array-filler.cpp):

    struct S { S() { throw 2; } };
    struct A { S s[2]; };
    static_assert(!noexcept(A{}));   // fails

An array element an initializer list does not initialize explicitly is
initialized by the list's *array filler* (here a call of `S::S()`), which is
potentially-throwing, so `A{}` is a potentially-throwing expression
([except.spec]/6.2) and `noexcept(A{})` is false.  Clang says true.

`Sema::canThrow` treats `InitListExpr` like any expression whose children
decide, and `InitListExpr::children()` holds only the explicit
initializers, not the array filler.

On this branch a contract predicate's evaluation_exception handling is
decided by `Sema::canThrow` (CLANG-587), so a predicate such as
`pre(((void)A{}, x > 0))` lets `S::S()`'s exception escape the check instead
of reaching the violation handler.  The partial walk CodeGen used before had
the same hole, for the same reason.

Its `verify.sh` case is `CLANG-664` in `verify-cases.txt`.

Fixed on this branch by a case of its own for `InitListExpr` in
`Sema::canThrow` that merges the filler's answer into the children's; the
regression tests are `clang/test/SemaCXX/noexcept-array-filler.cpp` (which
replaced the watch test `clang/test/Contracts/OpenBugs/noexcept-array-filler.cpp`)
and `clang/test/Contracts/predicate-throw-array-filler.cpp`.  Mirrored in
the GCC fork as GCC-681 (GCC's `noexcept` is wrong the same way).
