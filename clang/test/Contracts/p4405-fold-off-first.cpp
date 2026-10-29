// RUN: %clang_cc1 -std=c++26 -fno-contracts-prove-fold -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %clang -std=c++26 -fno-contracts-prove-fold -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fsyntax-only -### %s 2>&1 | FileCheck %s --check-prefix=DRIVER

// P4405: -fno-contracts-prove-fold written before -fcontracts-p4405 still
// turns folding off (p4405-fold-off.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-off-first.C (the
// driver line is Clang's own)

void f() {
  contract_assert(true);        // expected-error {{contract assertion cannot be proven}}
  contract_assert(false);       // expected-error {{contract assertion cannot be proven}}
}

// DRIVER: "-cc1"
// DRIVER-SAME: "-fcontracts-p4405"
// DRIVER-SAME: "-fno-contracts-prove-fold"
