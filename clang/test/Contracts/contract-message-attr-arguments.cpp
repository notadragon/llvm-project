// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// [[clang::contract_message]] takes exactly one argument, the message, in a
// template as out of one.  There is no [[clang::contract_semantic]]: a
// contract assertion's semantic comes from the configuration and the command
// line.
//
// No GCC mirror: GCC has no counterpart attributes.

void f(int x) {
  contract_assert [[clang::contract_message("m")]] (x > 0);
  contract_assert [[clang::contract_message("m", true)]] (x > 0); // expected-error {{attribute takes one argument}}
  contract_assert [[clang::contract_semantic("observe")]] (x > 0); // expected-warning {{unknown attribute}}
}

template <bool B> void g(int x) {
  contract_assert [[clang::contract_message("m", B)]] (x > 0); // expected-error {{attribute takes one argument}}
}
