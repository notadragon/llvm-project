// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose %libcxx_flags -o %t -Xclang -verify
// RUN: not --crash %t | FileCheck %s
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// D4405R0, "Analyzing Assertions With Restricted Semantics": built to ignore
// contract assertions, the terminating precondition is still enforced
// (ignore is not in its allowed set), and a configured analysis semantic
// of diagnose, which terminating does not allow, falls back to ignore, so
// nothing is diagnosed.  The paper's illustration
// (p4405-paper-analysis-terminating.cpp configures prove).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-terminating-run.C

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
  pre<terminating>(p >= 0 && p <= 100)
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
  return lookup(v);  // Error if proven, otherwise enforced
}

int main()
{
  return lookup_clamped(1000) + lookup_raw(1000);
}

// CHECK: violation: p >= 0 && p <= 100
