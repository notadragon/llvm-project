// RUN: %clang -std=c23 -fcontracts-p4299 -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// In C, contract_assert is an ordinary identifier (P4299 spells the keyword
// _ContractAssert), so a function of that name can be declared and called.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-assert-identifier.c

int contract_assert(int);

void f(int x) { contract_assert(x > 0); }
