// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose %libcxx_flags -o %t -Xclang -verify
//
// RUN: %{run} -fcontracts-p4451
// RUN: not --crash %t | FileCheck %s
// RUN: %{run} -fcontracts-p3400
// RUN: not --crash %t | FileCheck %s
// RUN: %{run} -fcontracts-p3400 -fcontracts-p4451
// RUN: not --crash %t | FileCheck %s

// expected-no-diagnostics

// P4451, D4405: built to ignore contract assertions, a terminating
// precondition is still enforced (ignore is not in its allowed set), and a
// configured analysis semantic of diagnose, which terminating does not
// allow, falls back to ignore, so nothing is diagnosed.
// p4405-paper-analysis-terminating-run.cpp under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-prove-run.C (and its
// -both variant; p4405-paper-analysis-terminating-run.C is the -p3400 one)

#include <contracts>  // `contract_violation`, `terminating`
#include <cstdio>     // `std::printf`, `std::fflush`

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
