// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-fold -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fno-contracts-prove-fold -c %s 2>&1 | FileCheck %s --check-prefix=DRIVER

// P4405: -fno-contracts-prove-fold turns the folding strategy off, giving
// stage 1's behaviour back: every evaluation with prove is an error, "cannot
// be proven", even when its predicate is true or false.  The -fno- form
// wins whichever side of -fcontracts-p4405 it is written on
// (p4405-fold-off-first.cpp).  The driver passes it on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-off.C (the driver
// line is Clang's own)

void f() {
  contract_assert(true);        // expected-error {{contract assertion cannot be proven}}
  contract_assert(false);       // expected-error {{contract assertion cannot be proven}}
}

// DRIVER: "-cc1"
// DRIVER-SAME: "-fcontracts-p4405"
// DRIVER-SAME: "-fno-contracts-prove-fold"
