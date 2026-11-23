// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3099 -fsyntax-only -verify %s

// Error when both [[clang::contract_message]] attribute and syntactic message.

void f(int x)
  pre [[clang::contract_message("attr msg")]] (x > 0, "syntactic msg") // expected-error {{cannot use both a syntactic diagnostic message and '[[clang::contract_message]]' on the same contract assertion}}
{}

// Attribute alone is fine
void g(int x)
  pre [[clang::contract_message("attr msg")]] (x > 0) // OK
{}

// Syntactic message alone is fine
void h(int x) pre(x > 0, "syntactic msg") {} // OK
