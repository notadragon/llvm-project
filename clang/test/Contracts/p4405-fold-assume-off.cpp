// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fno-contracts-prove-assume -c %s 2>&1 | FileCheck %s --check-prefix=DRIVER

// P4405: -fno-contracts-prove-assume is accepted and leaves folding as it
// was (Runnable/p4405-fold-assume.cpp).  The driver passes it on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-assume-off.C (the
// driver line is Clang's own)

void f(int x) {
  contract_assert(true);
  contract_assert(false);       // expected-error {{contract predicate is always false}}
  contract_assert(x > 0);       // expected-error {{contract assertion cannot be proven}}
}

// DRIVER: "-cc1"
// DRIVER-SAME: "-fcontracts-p4405"
// DRIVER-SAME: "-fno-contracts-prove-assume"
