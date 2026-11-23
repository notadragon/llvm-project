// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A reinterpret_cast is not allowed in a core constant expression
// ([expr.const]), in its reference form as in its pointer form.  Clang
// rejects all three; GCC accepts the two reference forms (stock, gnu_gcc
// GCC-713, deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-reference-reinterpret-cast.C

float g = 1.0f;
constexpr int &r = reinterpret_cast<int &>(g); // expected-error {{constexpr variable 'r' must be initialized by a constant expression}} \
                                               // expected-note {{reinterpret_cast is not allowed in a constant expression}}
constexpr bool f() {
  float x = 1.0f;
  int &i = reinterpret_cast<int &>(x); // expected-note {{reinterpret_cast is not allowed in a constant expression}}
  (void)i;
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}

// The pointer form.
constexpr bool h() {
  float x = 1.0f;
  int *i = reinterpret_cast<int *>(&x); // expected-note {{reinterpret_cast is not allowed in a constant expression}}
  (void)i;
  return true;
}
static_assert(h()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'h()'}}
