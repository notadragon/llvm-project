// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Binding a reference through a past-the-end pointer is undefined
// ([expr.unary.op]/1: the pointer points to no object), so it is not a core
// constant expression.  Clang rejects it; GCC accepts it (stock, gnu_gcc
// GCC-763, deferred), and its watch test is the program below.  The address
// form `&*(a + 1)`, which Clang accepts, is CLANG-712, and the dangling form
// is CLANG-717 (OpenBugs/).  Reading through either pointer is rejected:
// the controls at the end.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-through-invalid-pointer.C

constexpr bool f() {
  int a[1] = {0};
  int *p = a + 1;
  int &r = *p; // expected-note {{read of dereferenced one-past-the-end pointer is not allowed in a constant expression}}
  (void)r;
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}

// Controls (the passing halves of GCC's GCC-785 and GCC-786 watch tests):
// reading through a past-the-end or a dangling pointer is rejected.
constexpr int g() { int a[1] = {0}; return *(a + 1); } // expected-note {{read of dereferenced one-past-the-end pointer is not allowed in a constant expression}}
static_assert(g() == 0); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'g()'}}
constexpr int u() {
  int *p;
  { int y = 1; p = &y; } // expected-note {{declared here}}
  return *p; // expected-note {{read of object outside its lifetime}}
}
static_assert(u() == 1); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'u()'}}
