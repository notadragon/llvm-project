// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -emit-llvm-only %s 2>&1 | FileCheck %s

// D4405: -fcontract-analysis-semantic=prove without -fcontracts-p4405 is not
// allowed; the compiler says so and the analysis evaluation is ignore, so
// nothing is diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-without-flag.C

// CHECK: warning: '-fcontract-analysis-semantic=prove' requires '-fcontracts-p4405'; 'ignore' is used instead
// CHECK-NOT: cannot be proven
// CHECK-NOT: the contract configuration asks for

int f(int x)
  pre(x > 0)
{
  contract_assert(x != 3);
  return x;
}
