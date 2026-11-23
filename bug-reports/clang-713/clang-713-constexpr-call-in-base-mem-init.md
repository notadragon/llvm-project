# CLANG-713: A member call (or `dynamic_cast`) on `*this` in a base-class mem-initializer is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** [#211286](https://github.com/llvm/llvm-project/issues/211286) (exact for the call; fix PR #219358 open; `dynamic_cast` partial)
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:class.base.init.mem.fun`
**Watch tests:** `clang/test/Contracts/OpenBugs/constexpr-call-in-base-mem-init.cpp` and `clang/test/Contracts/OpenBugs/constexpr-dynamic-cast-in-base-mem-init.cpp` (XFAIL)

## Reproducer

[`clang-713-constexpr-call-in-base-mem-init.cpp`](clang-713-constexpr-call-in-base-mem-init.cpp), at `-std=c++26 -fsyntax-only`:

    struct A { int a; constexpr A(int x) : a(x) {} };
    struct B : A {
      constexpr int g() const { return 1; }
      constexpr B() : A(g()) {}     // undefined [class.base.init]
    };
    static_assert((B{}, true));

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  A virtual call and `typeid` there are rejected; `dynamic_cast` is accepted.

Found by the D4277R1 per-UB review (2026-10-12).
