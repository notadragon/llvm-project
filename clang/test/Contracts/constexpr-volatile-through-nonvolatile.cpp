// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Reading a volatile object through a non-volatile glvalue is undefined
// ([dcl.type.cv]/5), so it is not a core constant expression.  Clang rejects
// it; GCC accepts it (stock, gnu_gcc GCC-744, PR126359, deferred), and its
// watch test is the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-volatile-through-nonvolatile.C

constexpr int f() {
  volatile int v = 1; // expected-note {{volatile object declared here}}
  return const_cast<int &>(v); // expected-note {{read of volatile object 'v' is not allowed in a constant expression}}
}
static_assert(f() == 1); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
