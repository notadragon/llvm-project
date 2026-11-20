// P3100: vendor macro is defined when -fcontracts-allow-assume is active.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-allow-assume -fsyntax-only -verify %s
// expected-no-diagnostics

#ifndef __clang_contracts_allow_assume
#error "__clang_contracts_allow_assume not defined"
#endif
