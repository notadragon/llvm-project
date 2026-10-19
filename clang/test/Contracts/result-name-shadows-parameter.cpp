// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fsyntax-only -verify %s

// A postcondition result name that conflicts with a parameter is ill-formed
// ([basic.scope.contract]) whatever the evaluation semantic and whether or not
// the declaration is a definition.  The lambda-capture case is in
// result-name-shadows-lambda-capture.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-shadows-parameter.C

int f(int r) post(r : r > 0) { return r; } // expected-error {{declaration of result name 'r' shadows parameter}} expected-note {{previous declaration is here}}
int g(int r) post(r : r > 0);              // expected-error {{declaration of result name 'r' shadows parameter}} expected-note {{previous declaration is here}}
