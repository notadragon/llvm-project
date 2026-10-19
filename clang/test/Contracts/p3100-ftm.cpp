// P3100: gate macro is defined when -fcontracts-p3100 is active.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s
// expected-no-diagnostics

#ifndef __clang_contracts_p3100
#error "__clang_contracts_p3100 not defined"
#endif

// Unadopted: the value is the date of the implemented revision.
#if __clang_contracts_p3100 <= 0
#error "__clang_contracts_p3100 not positive"
#endif
