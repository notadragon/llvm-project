// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A violated precondition or postcondition of a constexpr constructor is
// checked during constant evaluation, so the initializer is not a constant
// expression.  The destructor shape is constexpr-dtor-contracts.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-cdtor-contracts.C

struct S {
  int v;
  constexpr S(int x) pre(x > 0) : v(x) {} // expected-error {{contract failed during execution of constexpr function}}
};
constexpr S s(-1); // expected-error {{constexpr variable 's' must be initialized by a constant expression}} expected-note {{in call to 'S(-1)'}}

struct T {
  int v;
  constexpr T(int x) post(v == 99) : v(x) {} // expected-error {{contract failed during execution of constexpr function}}
};
constexpr T t(1); // expected-error {{constexpr variable 't' must be initialized by a constant expression}} expected-note {{in call to 'T(1)'}}

constexpr S ok(1); // OK
