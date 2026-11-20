// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Using a heap object after an explicit destructor call -- reading a member,
// a virtual call, a conversion to a virtual base, or a delete through a
// virtual destructor -- is not a core constant expression: the object is
// outside its lifetime.  Clang rejects each; the virtual-base conversion is
// rejected at the conversion itself by CLANG-1's fix (stock Clang rejects it
// only at the delete that follows).  GCC ICEd its checking builds on these
// (gnu_gcc GCC-710, fixed there).  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-heap-after-dtor.C

struct T { int x = 0; };
constexpr int f() {
  T *p = new T; // expected-note {{heap allocation performed here}}
  p->~T();
  int r = p->x; // expected-note {{read of object outside its lifetime}}
  delete p;
  return r;
}
static_assert(f() == 0); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}

struct V {
  constexpr virtual int g() const { return 1; }
  constexpr virtual ~V() {}
};
struct W : V {};
constexpr int h1() {
  V *p = new W; // expected-note {{heap allocation performed here}}
  p->~V();
  int r = p->g(); // expected-note {{member call on object outside its lifetime}}
  delete p;
  return r;
}
static_assert(h1() == 1); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 'h1()'}}

struct VB { int v = 1; };
struct VD : virtual VB { constexpr ~VD() {} };
constexpr int h3() {
  VD *p = new VD; // expected-note {{heap allocation performed here}}
  p->~VD();
  VB *b = p; // expected-note {{conversion to a virtual base of object outside its lifetime}}
  (void)b;
  delete p;
  return 1;
}
static_assert(h3() == 1); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 'h3()'}}

constexpr int h4() {
  V *p = new W;
  p->~V();
  delete p; // expected-note {{whose lifetime has already ended}}
  return 1;
}
static_assert(h4() == 1); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 'h4()'}}
