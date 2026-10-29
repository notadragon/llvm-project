// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fsyntax-only -verify %s

// P4283: Error cases.

// requires clause on non-templated function is ill-formed.
void f(int x)
  pre requires(true) (x > 0); // expected-error {{requires clause on contract assertion is only allowed on templated functions}}

// requires clause on non-template contract_assert.
void g(int x) {
  contract_assert requires(true) (x > 0); // expected-error {{requires clause on contract assertion is only allowed on templated functions}}
}
