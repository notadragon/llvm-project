// clang-09-member-address-before-ctor.cpp                            -*-C++-*-
//
// CLANG-9: forming the address of a non-virtual-base or direct member before
// its enclosing object's non-trivial constructor begins is accepted in a
// constant expression; it must be rejected.
//
//   clang++ -std=c++26 -fsyntax-only clang-09-member-address-before-ctor.cpp
//     -> both cases below are accepted; each must be an error
//
// [class.cdtor]/1: "For an object with a non-trivial constructor, referring
// to any non-static member or base class of the object before the
// constructor begins execution results in undefined behavior."  There is no
// carve-out for virtual versus non-virtual bases, and none for forming an
// address versus reading.  CLANG-1 in this directory covers the virtual-base
// shape of the same defect, which this branch now rejects (CLANG-1's fix);
// upstream Clang still accepts it.  These two shapes are the still-open
// remainder.
//
// NO CONTRACTS ARE INVOLVED.  This is a pure core-language constexpr
// evaluator issue.
//
// GCC shares this general under-diagnosis too (rows b/c of the same defect,
// tracked as GCC-2 in the gnu_gcc fork).

// ---- (b) non-virtual base member, non-trivial ctor, form address. -------
namespace non_virtual_base_member {
struct W { int j; };
struct X : W {
  constexpr X () { }              // user-provided -> non-trivial
};
struct Y {
  int *p;
  X x;
  constexpr Y () : p (&x.j) { }   // must be an error: x is not yet built
};
constexpr int f () { Y y; return y.p != nullptr; }
static_assert ((f (), true));
}

// ---- (c) direct member, non-trivial ctor, form address. -----------------
namespace direct_member {
struct Z {
  int k;
  constexpr Z () : k (0) { }      // user-provided -> non-trivial
};
struct Y {
  int *p;
  Z z;
  constexpr Y () : p (&z.k) { }   // must be an error: z is not yet built
};
constexpr int f () { Y y; return y.p != nullptr; }
static_assert ((f (), true));
}
