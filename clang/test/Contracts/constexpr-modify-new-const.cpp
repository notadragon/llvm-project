// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Modifying an object created by `new const int` is undefined
// ([dcl.type.cv]/4), so it is not a core constant expression.  Clang rejects
// it; GCC accepts it (stock, gnu_gcc GCC-754, deferred), and its watch test
// is the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-modify-new-const.C

constexpr int f() {
  const int *p = new const int(3);
  *const_cast<int *>(p) = 4; // expected-note {{modification of object of const-qualified type 'const int'}}
  int r = *p;
  delete p;
  return r;
}
static_assert(f() == 4); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
