// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-analysis-semantic=ignore -emit-llvm-only -verify %s
// expected-no-diagnostics

// D4405: -fcontract-analysis-semantic=ignore is the default: no analysis.
// Given after =prove, the last one wins.  A prove selected for the runtime
// evaluations by -fcontract-evaluation-semantic still moves to the analysis
// evaluation (the stronger of the two), as p4405-prove.cpp shows; here the
// runtime evaluations are ignore, so nothing is diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-ignore.C

int f(int x)
  pre(x > 0)
  post(r: r > 0)
{
  contract_assert(x != 3);
  return x;
}
