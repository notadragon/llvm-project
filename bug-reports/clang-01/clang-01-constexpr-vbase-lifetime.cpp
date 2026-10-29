// clang-01-constexpr-vbase-lifetime.cpp                             -*-C++-*-
//
// CLANG-1: the constexpr evaluator accepts a derived-to-virtual-base
// conversion on an object that is outside its lifetime.
//
//   clang++ -std=c++26 -fsyntax-only clang-01-constexpr-vbase-lifetime.cpp
//     -> all three cases below are accepted; each must be an error
//
// Converting a pointer or glvalue to a VIRTUAL base consults the most-derived
// object's layout to compute the virtual base offset.  That is a use of the
// object, so doing it to an object that has been deleted, or whose
// construction has not begun, is undefined and is not a constant expression.
// A NON-virtual base conversion uses a static offset and does not access the
// object, so it stays valid -- see the positive controls at the end.
//
// Root cause: HandleLValueBase (clang/lib/AST/ExprConstant.cpp) takes the
// virtual-base branch -- CastToDerivedClass + getVBaseClassOffset -- with no
// object-liveness check, unlike the member-access and lvalue-to-rvalue paths.
//
// NO CONTRACTS ARE INVOLVED.  This is a pure core-language issue and is
// filable against upstream Clang as-is, even though contracts are not
// upstreamed.
//
// GCC rejects the first two ("use of allocated storage after deallocation in
// a constant expression") but shares the third under-diagnosis: that is row
// (a) of the four-row table in GCC-2 (gnu_gcc fork). Rows (b) and (c), which
// both compilers still accept, are CLANG-9 in this directory.

// ---- 1. Deleted object, conversion through a glvalue. -------------------
namespace deleted_glvalue {
struct B { };
struct D : virtual B { };
constexpr int f ()
{
  D *d = new D ();
  D &r = *d;
  delete d;
  B &b = r;          // must be an error: r is outside its lifetime
  (void) &b;
  return 0;
}
static_assert ((f (), true));
}

// ---- 2. Deleted object, conversion through a pointer. -------------------
namespace deleted_pointer {
struct B { };
struct D : virtual B { };
constexpr int f ()
{
  D *p = new D ();
  delete p;
  B *b = p;          // must be an error: p's object is outside its lifetime
  (void) b;
  return 0;
}
static_assert ((f (), true));
}

// ---- 3. Object whose construction has not begun. ------------------------
namespace before_construction {
struct W { int j; };
struct X : virtual W { };
struct Y {
  int *p;
  X x;
  constexpr Y () : p (&x.j) { }   // must be an error: x is not yet built
};
constexpr int f () { Y y; return y.p != nullptr; }
static_assert ((f (), true));
}

// ---- Positive controls: these must REMAIN constant expressions. ---------
namespace live_object_is_fine {
struct B { };
struct D : virtual B { };
constexpr int ok ()
{
  D d;
  B &b = d;
  (void) &b;
  return 0;
}
static_assert ((ok (), true));
}

namespace during_construction_is_fine {
struct B { };
struct D : virtual B {
  B *made;
  // The virtual base is constructed before the derived-class body runs.
  constexpr D () : made (this) { }
};
constexpr int ok () { D d; return d.made != nullptr; }
static_assert ((ok (), true));
}

namespace non_virtual_base_of_deleted_object_is_fine {
struct B { };
struct D : B { };    // non-virtual: static offset, no access to the object
constexpr int ok ()
{
  D *p = new D ();
  delete p;
  B *b = p;
  (void) b;
  return 0;
}
static_assert ((ok (), true));
}
