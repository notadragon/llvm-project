// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3098: the postcondition-captures feature-test macro is defined with the flag.
// (Language feature only; there is no library __cpp_lib_ counterpart.)
// (GCC mirror: g++.dg/contracts/cpp26/p3098-ftm.C.)

#ifndef __cpp_contracts_postcondition_captures
#error "__cpp_contracts_postcondition_captures not defined"
#endif

#if __cpp_contracts_postcondition_captures != 202606L
#error "__cpp_contracts_postcondition_captures has wrong value"
#endif
