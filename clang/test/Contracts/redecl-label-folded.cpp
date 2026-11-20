// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s

// The assertion-control specifiers of two declarations must satisfy the ODR
// as contract-control expressions (P3400): a different constexpr object, or
// a temporary, does not match even when the values are equal.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-label-folded.C

#include <contracts>
struct L { using assertion_control_object = L; };
constexpr L a{}, b{};

void f(int x) pre<a>(x > 0); // expected-note {{contract previously specified with a different assertion-control label}}
void g(int x) pre<a>(x > 0); // expected-note {{contract previously specified with a different assertion-control label}}

void f(int x) pre<b>(x > 0) {}   // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
void g(int x) pre<L{}>(x > 0) {} // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

// REQUIRES: contracts-libcxx
