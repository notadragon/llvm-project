// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Pins what GCC accepted until it was fixed there (upstream PR97647): deleting
// an object through a pointer to a base class whose destructor is not virtual
// is undefined ([expr.delete]/3) and so not a core constant expression, which
// GCC's constant evaluation accepted.  Clang rejects it, and this pins that.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-delete-nonvirtual-dtor.C

struct B { int a; constexpr ~B() {} };
struct D : B { int b; constexpr ~D() {} };

constexpr bool f() {
  B *b = new D;
  delete b; // expected-note {{delete of object with dynamic type 'D' through pointer to base class type 'B' with non-virtual destructor}}
  return true;
}

static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
