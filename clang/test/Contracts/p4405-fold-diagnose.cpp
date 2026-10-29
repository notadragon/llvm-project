// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-llvm-only -verify=quiet %s
// quiet-no-diagnostics

// P4405 stage 2, -fcontracts-prove-fold: as p4405-fold-prove.cpp with the
// diagnose semantic -- a -Wcontract-unproven warning, for "always false" as
// for "cannot be proven", and nothing for a predicate that folds to true;
// -Wno-contract-unproven silences both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-diagnose.C

int f(int x)
  pre(true)
  pre(x > 0)                            // expected-note {{the predicate of the contract assertion}}
  post(r: sizeof(r) == 0)               // expected-warning {{contract predicate is always false}}
{
  contract_assert(sizeof(int) >= 1);
  contract_assert(false);               // expected-warning {{contract predicate is always false}}
  contract_assert(x != 3);              // expected-warning {{contract assertion cannot be proven}}
  return x;
}

void ae2_calls(int y) {
  f(y); // expected-warning {{contract assertion cannot be proven}}
  f(1);
}
