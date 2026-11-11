// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A non-reference parameter odr-used in a postcondition must be const on
// every declaration of the function ([dcl.contract.func]; the note says this
// applies even to declarations without the postcondition), including a
// non-defining redeclaration.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-post-param-const.C

void f(const int x) post(x > 0); // expected-note {{odr-used in a postcondition here}}
void f(int x); // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}}
void f(const int x) {}

void g(const int x) post(x > 0);
void g(const int x);
void g(const int x) {}
