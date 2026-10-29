// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-fold -fcontracts-prove-fold -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s

// P4405: -fcontracts-prove-fold written explicitly, after
// -fno-contracts-prove-fold, turns folding back on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-explicit.C

void f() {
  contract_assert(true);
  contract_assert(false);       // expected-error {{contract predicate is always false}}
}
