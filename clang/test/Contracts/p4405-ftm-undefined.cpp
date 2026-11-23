// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// P4405: without -fcontracts-p4405 the feature-test macro is not defined
// (the umbrella -fcontracts-p3850 enables it, so use a bare -fcontracts).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-ftm-undefined.C

#ifdef __cpp_contracts_prove
#error "__cpp_contracts_prove defined without -fcontracts-p4405"
#endif

int f(int x) pre(x > 0) { return x; }
