// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fsyntax-only -verify %s
// expected-no-diagnostics

#ifndef __cpp_contracts_requires
#error "__cpp_contracts_requires not defined"
#endif

// Unadopted: the value is the date of the implemented revision.
#if __cpp_contracts_requires <= 0
#error "__cpp_contracts_requires not positive"
#endif
