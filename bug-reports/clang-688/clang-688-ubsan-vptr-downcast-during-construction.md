# CLANG-688: UBSan `vptr` reports a defined `static_cast` downcast performed during construction of the derived object

**Status:** Open here; open upstream
**Component:** compiler-rt:ubsan
**Upstream Link:** [#59060](https://github.com/llvm/llvm-project/issues/59060)
**Affects:** stock trunk `33891afe2ebb` and this branch's install of 2026-10-12
**Watch test:** `clang/test/Contracts/OpenBugs/vptr-downcast-during-construction.cpp` (XFAIL)

## Reproducer

[`clang-688-ubsan-vptr-downcast-during-construction.cpp`](clang-688-ubsan-vptr-downcast-during-construction.cpp), at `-std=c++26 -O0 -fsanitize=vptr`:

    struct B { virtual ~B() {} };
    struct E : B { E(); };
    struct F : E { int x = 0; };
    E::E() { (void)static_cast<F*>(static_cast<B*>(this)); }  // defined while an F is being built
    int main() { F f; }

Stock trunk and this branch report "downcast of address ... which does not
point to an object of type 'F' ... object is of type 'E'".  The `B` is a base
subobject of the `F` under construction, so [expr.static.cast] gives the
enclosing `F`; the constant evaluator accepts the constexpr form.  Under an
enforcing semantic routed through the integrated `vptr` check
(`ub:expr.static.cast.downcast.wrong.derived.type`,
`ub:expr.static.cast.base.class`) this terminates a valid program.  GCC has
the same false positive (GCC-717 there).

Found by the D4277R1 per-UB review (2026-10-12).
