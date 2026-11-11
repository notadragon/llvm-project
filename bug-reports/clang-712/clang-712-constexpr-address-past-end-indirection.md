# CLANG-712: `&*(a + N)` (indirection through a past-the-end pointer, address taken) is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** [#65119](https://github.com/llvm/llvm-project/issues/65119) (closed not_planned: a maintainer considers `&a[N]` well-defined)
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.unary.dereference (.indirection)`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-address-past-end-indirection.cpp` (XFAIL)

## Reproducer

[`clang-712-constexpr-address-past-end-indirection.cpp`](clang-712-constexpr-address-past-end-indirection.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool f() { int a[1] = {0}; int* p = &*(a + 1); return p != nullptr; }   // undefined per [expr.unary.op]
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Tracked as upstream's deliberate choice.

Found by the D4277R1 per-UB review (2026-10-12).
