// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify %s

// P4405: every evaluation with the diagnose semantic gets a
// -Wcontract-unproven warning, which is on by default -- a precondition, a
// postcondition and an assertion.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-diagnose.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-warning {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}

void ae2_calls(int y) {
  f(y); // expected-warning {{contract assertion cannot be proven}}
}
