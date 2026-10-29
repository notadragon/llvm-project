# CLANG-725: constant evaluation gives the wrong result for a defined `dynamic_cast` through a virtual base during construction

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `33891afe2ebb` and this branch's install of 2026-10-12; GCC (stock and ours) and Clang's `-fexperimental-new-constant-interpreter` are correct
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-dynamic-cast-vbase-construction.cpp` (XFAIL)

## Reproducer

[`clang-725-constexpr-dynamic-cast-vbase-construction.cpp`](clang-725-constexpr-dynamic-cast-vbase-construction.cpp), at `-std=c++26 -fsyntax-only`:

    struct V { virtual void f() {} };
    struct Bx : virtual V {
      bool good;
      constexpr Bx() { V* v = this; good = (dynamic_cast<Bx*>(v) == this); }
    };
    struct D : Bx { constexpr D() {} };
    constexpr bool t1() { D d; return d.good; }
    static_assert(t1());

Clang: "static assertion failed due to requirement 't1()'" -- the constant
evaluator computes `dynamic_cast<Bx*>(v) == this` as false.  The cast is
defined ([class.cdtor]: `v`'s type `V` is a base of the class under
construction, so the cast yields the `Bx` subobject), GCC evaluates it to
true, the bytecode interpreter (`-fexperimental-new-constant-interpreter`)
evaluates it to true, and at run time the program prints the right answer.
`typeid(*v) == typeid(*this)` in the same position is likewise wrong.  With
a non-virtual base `V` Clang is correct.

## Upstream search (2026-10-12)

GitHub issue search on llvm/llvm-project including closed issues (control
validated earlier the same day: a phrase from #211286's title returned it).
Queries: `constexpr dynamic_cast virtual base construction`; `constexpr
dynamic_cast "virtual base" wrong` (both return only #211286, unrelated).

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Constant evaluation of a dynamic_cast through a virtual base
during construction gives the wrong result

**Description:** The program below is well-defined; the static_assert must
pass (GCC and Clang's bytecode interpreter agree, and the run-time result is
correct), but Clang's constant evaluator evaluates the dynamic_cast to a
pointer other than `this` (and `typeid` to the wrong type) when the operand
is a virtual base of the class under construction.

    struct V { virtual void f() {} };
    struct Bx : virtual V {
      bool good;
      constexpr Bx() { V* v = this; good = (dynamic_cast<Bx*>(v) == this); }
    };
    struct D : Bx { constexpr D() {} };
    constexpr bool t1() { D d; return d.good; }
    static_assert(t1());

Found by the D4277R1 per-UB review (2026-10-12).
