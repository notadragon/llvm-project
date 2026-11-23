// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Pins what GCC accepted until it was fixed there (upstream PR126425): after
// an explicit destructor call has ended an object's lifetime, a
// delete-expression destroys it again.  That is not a core constant
// expression, which GCC's constant evaluation accepted.  Clang rejects it,
// and this pins that.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-heap-double-destroy.C

struct S { float f = 0; constexpr ~S() {} };

constexpr bool f() {
  S *p = new S;
  p->~S();
  delete p; // expected-note {{whose lifetime has already ended}}
  return true;
}

static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
