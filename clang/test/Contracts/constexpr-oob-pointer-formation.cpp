// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Forming a pointer more than one past the end of an array is undefined
// ([expr.add]/4.3), so it is not a core constant expression even when the
// pointer is brought back into range before it is used.  Clang rejects it at
// the formation; GCC accepts it (stock, gnu_gcc GCC-739, PR70151, PR111242,
// deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-oob-pointer-formation.C

constexpr int f() {
  int a[3] = {1, 2, 3};
  int *p = a + 4; // expected-note {{cannot refer to element 4 of array of 3 elements in a constant expression}}
  return *(p - 4);
}
static_assert(f() == 1); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
