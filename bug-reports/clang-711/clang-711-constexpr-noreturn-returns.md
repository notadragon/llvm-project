# CLANG-711: A `[[noreturn]]` function that returns is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** [#211272](https://github.com/llvm/llvm-project/issues/211272)
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:dcl.attr.noreturn.eventually.returns`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-noreturn-returns.cpp` (XFAIL)

## Reproducer

[`clang-711-constexpr-noreturn-returns.cpp`](clang-711-constexpr-noreturn-returns.cpp), at `-std=c++26 -fsyntax-only`:

    [[noreturn]] constexpr void g() {}           // warning only
    constexpr bool f() { g(); return true; }    // undefined: g returns
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Conforming after CWG2763/CWG2924 (implementation-defined whether this is a core constant expression), as noted on #211272; GCC rejects.

Found by the D4277R1 per-UB review (2026-10-12).
