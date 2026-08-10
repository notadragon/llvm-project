// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A conversion of a pointer or glvalue to a virtual base class consults the
// most-derived object's layout to compute the virtual base offset. That is a
// use of the object, so performing it on an object that is outside its
// lifetime -- one that has been deleted or whose construction has not yet
// begun -- is undefined behavior and is not a constant expression. A
// non-virtual base conversion uses a static offset and does not access the
// object, so it remains valid.

namespace DeletedGlvalueToVirtualBase {
struct B {};
struct D : virtual B {};
constexpr int f() {
  D *d = new D();
  D &r = *d;
  delete d;
  B &b = r; // expected-note {{dynamic_cast of heap allocated object that has been deleted}}
  (void)&b;
  return 0;
}
static_assert((f(), true)); // expected-error {{not an integral constant expression}} \
                            // expected-note {{in call to 'f()'}}
} // namespace DeletedGlvalueToVirtualBase

namespace DeletedPointerToVirtualBase {
struct B {};
struct D : virtual B {};
constexpr int f() {
  D *p = new D();
  delete p;
  B *b = p; // expected-note {{dynamic_cast of heap allocated object that has been deleted}}
  (void)b;
  return 0;
}
static_assert((f(), true)); // expected-error {{not an integral constant expression}} \
                            // expected-note {{in call to 'f()'}}
} // namespace DeletedPointerToVirtualBase

namespace MemberOfNotYetConstructedVirtualBase {
struct W {
  int j;
};
struct X : virtual W {};
struct Y {
  int *p;
  X x;
  // Forming &x.j requires the X -> W virtual base conversion, but x (and thus
  // its virtual base W) has not begun construction when p is initialized.
  constexpr Y() : p(&x.j) {} // expected-note {{dynamic_cast of object outside its lifetime is not allowed in a constant expression}}
};
constexpr int f() {
  Y y; // expected-note {{in call to 'Y()'}} expected-note {{declared here}}
  return y.p != nullptr;
}
static_assert((f(), true)); // expected-error {{not an integral constant expression}} \
                            // expected-note {{in call to 'f()'}}
} // namespace MemberOfNotYetConstructedVirtualBase

// ---- Positive controls: these must remain constant expressions. ----

namespace LiveObjectIsFine {
struct B {};
struct D : virtual B {};
constexpr int ok() {
  D d;
  B &b = d;
  (void)&b;
  D *p = new D();
  B *q = p;
  (void)q;
  delete p;
  return 0;
}
static_assert((ok(), true));
} // namespace LiveObjectIsFine

namespace DuringConstructionIsFine {
struct B {};
struct D : virtual B {
  B *made;
  // The virtual base is constructed before the derived-class body runs, so the
  // derived-to-virtual-base conversion here is valid.
  constexpr D() : made(this) {}
};
constexpr int ok() {
  D d;
  return d.made != nullptr;
}
static_assert((ok(), true));
} // namespace DuringConstructionIsFine

namespace NonVirtualBaseOfDeletedObjectIsUnaffected {
struct B {};
struct D : B {}; // non-virtual base: static offset, no object access
constexpr int ok() {
  D *p = new D();
  delete p;
  B *b = p; // fine: no use of the object's layout
  (void)b;
  return 0;
}
static_assert((ok(), true));
} // namespace NonVirtualBaseOfDeletedObjectIsUnaffected
