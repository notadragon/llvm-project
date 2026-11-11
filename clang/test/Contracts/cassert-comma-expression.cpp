// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3290 -Wno-unused-value %libcxx_flags -fsyntax-only -Xclang -verify %s

// With the contract-integrated assert, assert(a, b) is ill-formed: the
// argument is not a single assignment-expression ([assertions.assert] p3).
// A parenthesized comma expression, and a template argument list containing a
// comma, are still accepted.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/cassert-comma-expression.C

#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>

template <typename T, typename U> constexpr bool same = false;
template <typename T> constexpr bool same<T, T> = true;

void f(int a, int b) {
  assert(a, b); // expected-error {{no matching function for call to '__assert_single_argument'}}
               // expected-note@*:* {{candidate function template not viable: requires 1 argument, but 2 were provided}}
  assert((a, b));
  assert((same<int, int>));
  assert(same<int, int>);
}

// REQUIRES: contracts-libcxx
