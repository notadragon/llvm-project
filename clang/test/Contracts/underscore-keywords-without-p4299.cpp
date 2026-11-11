// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// _Pre / _Post / _ContractAssert are the C (P4299) spellings and are keywords
// only with -fcontracts-p4299, in either language: without it they are
// ordinary identifiers in C++ with contracts on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/underscore-keywords-without-p4299.C

int _Pre = 1, _Post = 2, _ContractAssert = 3;
int g() { return _Pre + _Post + _ContractAssert; }
