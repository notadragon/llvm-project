// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -O2 -fcontract-configuration-file=%S/p4405-paper-analysis-blanket.json %libcxx_flags -o %t -Xclang -verify
// RUN: not --crash %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405R0, "P3595R1 Analysis Evaluation Configuration": one analysis entry
// diagnoses every contract assertion that cannot be proven -- each
// precondition at each call -- while the runtime entry still enforces every
// one of them.  The paper's configuration; the program is the paper's
// illustration of it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-blanket.C

#include <contracts>  // `std::contracts::contract_violation`
#include <cstdio>  // `std::printf`, `std::fflush`

// A violation handler that reports each violation; with *enforce* the
// program then terminates.
void handle_contract_violation(const std::contracts::contract_violation& v)
{
  std::printf("violation: %s\n", v.comment());
  std::fflush(stdout);
}

int checked(int x)
  pre(x > 0)  // expected-note {{the predicate of the contract assertion}}
{
  return x;
}

int caller_ok(int y)
{
  if (y > 0)
    return checked(y);  // OK, proven
  return 0;
}

int caller_bad(int y)
{
  return checked(y);  // OK, warns: cannot be proven expected-warning {{contract assertion cannot be proven}}
}

int main(int argc, char**)
{
  caller_ok(argc);
  return caller_bad(-argc);  // Violation at runtime
}

// CHECK: violation: x > 0
