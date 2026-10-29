// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// delete[] through a pointer to a base class is undefined ([expr.delete]/3),
// so it is not a core constant expression even when the derived class has
// the size of the base.  Clang rejects it (the operand is not the array);
// GCC accepts it (stock, gnu_gcc GCC-753, deferred), and its watch test is
// the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-array-delete-via-base.C

struct B { int i = 0; };
struct D : B {}; // same size as B
constexpr bool f() {
  B *p = new D[2];
  delete[] p; // expected-note {{delete of pointer to subobject}}
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
