// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// Two declarations whose postconditions differ only in the presence of a
// result-name-introducer are ill-formed ([dcl.contract.func]), in either
// order.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-result-name-presence.C

bool b1;
int g() post(r : b1); // expected-note {{contract previously specified with result name}}
int g() post(b1);     // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
int h() post(b1);     // expected-note {{contract previously specified without result name}}
int h() post(r : b1); // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
