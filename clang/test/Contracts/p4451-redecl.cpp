// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=expected,p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=expected,p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=expected,p3400
// REQUIRES: contracts-libcxx

// P4451 [dcl.contract.func]: two function contract assertions are the same
// only if both have an assertion-control-specifier or neither does.  Under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone (where the specifier is
// the P3400 label, so the labels differ) and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-redecl.C (and its
// -p3400 and -both variants)

#include <contracts>  // `terminating`

void f(int x) pre<terminating>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier}} p3400-note {{contract previously specified with a different assertion-control label}}
void f(int x) pre(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

void g(int x) pre(x > 0); // p4451-note {{contract previously specified without an assertion-control-specifier}} p3400-note {{contract previously specified with a different assertion-control label}}
void g(int x) pre<terminating>(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

void h(const int x) pre<terminating>(x > 0) post<terminating>(x > 0);
void h(const int x) pre<terminating>(x > 0) post<terminating>(x > 0) { }

struct S {
  void m(int x) pre<terminating>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier}} p3400-note {{contract previously specified with a different assertion-control label}}
  void n(int x) pre<terminating>(x > 0);
};
void S::m(int x) pre(x > 0) { } // expected-error {{method out-of-line definition differs in contract specifier sequence}} expected-note {{in contract specified here}}
void S::n(int x) pre<terminating>(x > 0) { }
