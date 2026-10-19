// RUN: %clang_cc1 -std=c++98 -fcontracts -fsyntax-only -verify -verify-ignore-unexpected %s
// expected-no-diagnostics

// Before C++11 contract_assert is an ordinary identifier, even with
// -fcontracts, so a function of that name can be declared and called.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-cxx98.C

void f(int x) { contract_assert(x > 0); }
