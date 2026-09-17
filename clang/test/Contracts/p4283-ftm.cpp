// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fsyntax-only -verify %s
// expected-no-diagnostics

#ifndef __cpp_contracts_requires
#error "__cpp_contracts_requires not defined"
#endif

#if __cpp_contracts_requires != 202606L
#error "__cpp_contracts_requires has wrong value"
#endif
