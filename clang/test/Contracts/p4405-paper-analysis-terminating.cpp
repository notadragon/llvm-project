// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Analyzing Assertions With Restricted Semantics": terminating
// allows prove, so with the analysis evaluations configured prove its
// precondition must be proven at each call -- the clamped call is, the raw
// one is an error -- and it is still enforced at run time
// (p4405-paper-analysis-terminating-run.cpp).  The paper's illustration.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-terminating.C

#include <contracts>  // `terminating`, `contract_violation`
#include <cstdio>  // `std::printf`, `std::fflush`

using std::contracts::labels::terminating;

// A violation handler that reports each violation; with *enforce* the
// program then terminates.
void handle_contract_violation(const std::contracts::contract_violation& v)
{
  std::printf("violation: %s\n", v.comment());
  std::fflush(stdout);
}

int table[101];

int lookup(int p)
  pre<terminating>(p >= 0 && p <= 100)  // expected-note {{the predicate of the contract assertion}}
{
  return table[p];
}

int lookup_clamped(int v)
{
  if (v < 0)
    v = 0;
  if (v > 100)
    v = 100;
  return lookup(v);  // OK, proven
}

int lookup_raw(int v)
{
  return lookup(v);  // Error if proven, otherwise enforced expected-error {{contract assertion cannot be proven}}
}
