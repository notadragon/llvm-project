// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A result binding is a local entity ([basic.pre]) and so can be named in a
// simple-capture.  The constant-evaluation shape is
// constexpr-result-name-in-lambda.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-in-lambda.C

int fc(const int x) post(r : [r] { return r > 0; }()) { return x; }
