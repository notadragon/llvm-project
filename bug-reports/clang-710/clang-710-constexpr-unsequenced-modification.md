# CLANG-710: An unsequenced modification and read of a scalar is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** [#37768](https://github.com/llvm/llvm-project/issues/37768) (#108424 duplicate)
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:intro.execution.unsequenced.modification`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-unsequenced-modification.cpp` (XFAIL)

## Reproducer

[`clang-710-constexpr-unsequenced-modification.cpp`](clang-710-constexpr-unsequenced-modification.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int f() { int i = 0; i = i++ + i; return i; }   // undefined
    static_assert(f() >= 0);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Only `-Wunsequenced`; GCC accepts too (PR87275).

Found by the D4277R1 per-UB review (2026-10-12).
