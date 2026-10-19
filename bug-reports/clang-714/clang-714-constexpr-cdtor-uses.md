# CLANG-714: Uses of an object under construction through a not-yet-constructed path ([class.cdtor]) are not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:class.cdtor.convert.pointer, .dynamic.cast, .typeid, .virtual.not.x, .form.pointer, class.cdtor.after.dtor (.address)`
**Watch tests:** `clang/test/Contracts/OpenBugs/constexpr-cdtor-dynamic-cast.cpp` and `clang/test/Contracts/OpenBugs/constexpr-cdtor-convert-virtual-base.cpp` (XFAIL)

## Reproducer

[`clang-714-constexpr-cdtor-uses.cpp`](clang-714-constexpr-cdtor-uses.cpp), at `-std=c++26 -fsyntax-only`:

    struct V { virtual void f() {} };
    struct A : virtual V {};
    struct Bx : virtual V {
      bool ok;
      constexpr Bx(A* a) : ok(dynamic_cast<Bx*>(a) != nullptr) {} // UB [class.cdtor]/6
    };
    struct D : A, Bx { constexpr D() : Bx(this) {} };
    static_assert((D{}, true));

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  Related #211286.  Converting `this` to a virtual base of a base class whose construction has not begun ([class.cdtor]/3; gnu_gcc GCC-759's program, `struct E : CBase, DBase { constexpr E() : DBase(this) {} };` with `DBase : virtual A` taking an `A *`) is accepted too; the second watch test pins it.

## Upstream search (2026-10-12)

GitHub issue search on llvm/llvm-project including closed issues, control validated (a phrase from #211286's title returned it).  Queries: `constexpr "class.cdtor"`; `constexpr "under construction" virtual`; `constexpr "under construction"`; `constexpr virtual call during construction wrong`; `dynamic_cast constexpr virtual base`; `typeid constexpr virtual base`; `constexpr typeid construction`.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Uses of an object under construction through a not-yet-constructed path ([class.cdtor]) are not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([class.cdtor]; D4277R1 category `ub:class.cdtor.convert.pointer`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Related #211286.

    struct V { virtual void f() {} };
    struct A : virtual V {};
    struct Bx : virtual V {
      bool ok;
      constexpr Bx(A* a) : ok(dynamic_cast<Bx*>(a) != nullptr) {} // UB [class.cdtor]/6
    };
    struct D : A, Bx { constexpr D() : Bx(this) {} };
    static_assert((D{}, true));

Found by the D4277R1 per-UB review (2026-10-12).
