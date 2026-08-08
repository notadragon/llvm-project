// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// E3: negative gate -- without -fcontracts-p4283 the requires-on-contracts
// feature-test macro is not defined (the umbrella -fcontracts-p3850 enables it,
// so use a bare -fcontracts here).
// (GCC mirror: g++.dg/contracts/cpp26/p4283-ftm-undefined.C.)

#ifdef __cpp_contracts_requires
#error "__cpp_contracts_requires defined without -fcontracts-p4283"
#endif

int f(int x) pre(x > 0) { return x; }
