// RUN: %clang_cc1 -std=c++26 -fcontracts-p3850 -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s

// P4405: the umbrella -fcontracts-p3850 implies -fcontracts-p4405, and so
// the folding strategy.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-umbrella.C

void f(int x) {
  contract_assert(true);
  contract_assert(false);       // expected-error {{contract predicate is always false}}
  contract_assert(x > 0);       // expected-error {{contract assertion cannot be proven}}
}
