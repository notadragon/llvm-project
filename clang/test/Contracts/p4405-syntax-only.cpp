// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fsyntax-only -verify %s
// expected-no-diagnostics

// P4405: prove and diagnose are diagnosed where a check would be emitted,
// so -fsyntax-only, which emits nothing, diagnoses nothing.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-syntax-only.C

int f(int x) pre(x > 0) post(r: r > 0)
{
  contract_assert(x != 3);
  return x;
}
