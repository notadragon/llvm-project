# CLANG-689: A one-past-the-end pointer converted to a non-virtual base is rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend
**Upstream Link:** [#43201](https://github.com/llvm/llvm-project/issues/43201) (open); related #223064
**Affects:** stock trunk `33891afe2ebb` and this branch's install of 2026-10-12
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-past-end-nonvirtual-base.cpp` (XFAIL)

## Reproducer

[`clang-689-constexpr-past-end-nonvirtual-base.cpp`](clang-689-constexpr-past-end-nonvirtual-base.cpp), at `-std=c++26 -fsyntax-only`:

    struct B { int b = 1; };
    struct NV : B { int x = 2; };
    constexpr bool t() { NV ds[1]; NV* p = ds + 1; B* b = p; return b != nullptr; }
    static_assert(t());

Stock trunk (since 18 or earlier) and this branch reject ("cannot access base
class of pointer past the end of object"); GCC accepts.  [conv.ptr] makes only
the virtual-base case undefined; whether the non-virtual case is defined for a
pointer that points to no `NV` object is arguable, which is why #43201 is
still open.  Recorded so the D4277R1 tables do not count this rejection as a
check.

Found by the D4277R1 per-UB review (2026-10-12).
