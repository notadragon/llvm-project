// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// The postcondition parameter rule's reason for a function parameter: a
// non-const function pointer breaks it by not being const, and a parameter
// declared with function type breaks it by having function type (the
// declared type, as for arrays: after adjustment it is a pointer).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-function-param-reason.C,
// which covers only the function-pointer case (GCC says "must be const" for
// both).

void f2(int (*g)()) post(g != nullptr) {} // expected-error {{parameter 'g' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
void f4(int g()) post(g != nullptr) {}    // expected-error {{parameter 'g' referenced in contract postcondition cannot have a function type}} expected-note {{declared here}}
