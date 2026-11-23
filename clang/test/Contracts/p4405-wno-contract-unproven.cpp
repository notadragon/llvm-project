// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-llvm-only -verify %s
// expected-no-diagnostics

// P4405: -Wno-contract-unproven silences the diagnose semantic's warning.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-wno-contract-unproven.C

int f(int x)
  pre(x > 0)
  post(r: r > 0)
{
  contract_assert(x != 3);
  return x;
}
