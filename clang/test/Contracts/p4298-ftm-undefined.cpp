// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// E2: negative gate -- without -fcontracts-p4298 the nonthrowing-semantics
// feature-test macro is not defined (the umbrella -fcontracts-p3850 enables it,
// so use a bare -fcontracts here).
// (GCC mirror: g++.dg/contracts/cpp26/p4298-ftm-undefined.C.)

#ifdef __cpp_contracts_nonthrowing_semantics
#error "__cpp_contracts_nonthrowing_semantics defined without -fcontracts-p4298"
#endif

int f(int x) pre(x > 0) { return x; }
