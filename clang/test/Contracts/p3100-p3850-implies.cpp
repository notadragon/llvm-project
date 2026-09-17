// P3100: -fcontracts-p3850 implies -fcontracts-p3100.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fsyntax-only -verify %s
// expected-no-diagnostics

#ifndef __clang_contracts_p3100
#error "-fcontracts-p3850 should imply -fcontracts-p3100"
#endif
