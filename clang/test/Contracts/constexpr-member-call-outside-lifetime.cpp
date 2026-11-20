// RUN: %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// Calling a member function on an object outside its lifetime is undefined
// ([basic.life]/7), so it is not a core constant expression even when the
// function reads nothing.  Clang rejects it; GCC accepts it (stock, gnu_gcc
// GCC-751, deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-member-call-outside-lifetime.C

#include <memory>

struct S {
  int i = 0;
  constexpr int g() const { return 7; }
  constexpr ~S() {}
};
constexpr int f() {
  S s; // expected-note {{declared here}}
  s.~S();
  int r = s.g(); // expected-note {{member call on object outside its lifetime}}
  std::construct_at(&s);
  return r;
}
static_assert(f() == 7); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
