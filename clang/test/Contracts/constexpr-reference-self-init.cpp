// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A reference used in its own initializer refers to no object ([basic.life],
// [dcl.ref]), which is undefined, so it is not a core constant expression.
// Clang rejects it; GCC accepts it (stock, gnu_gcc GCC-746, PR119339,
// deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-self-init.C

constexpr int &id(int &r) { return r; }
constexpr bool f() {
  int &r = id(r); // expected-warning {{reference 'r' is not yet bound to a value when used within its own initialization}} \
                  // expected-note {{use of reference outside its lifetime is not allowed in a constant expression}}
  (void)r;
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
