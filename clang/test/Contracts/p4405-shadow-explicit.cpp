// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-shadow -fcontracts-prove-shadow -fno-contracts-prove-fold -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fno-contracts-prove-shadow -fcontracts-prove-shadow -c %s 2>&1 | FileCheck %s
// REQUIRES: x86-registered-target

// P4405 stage 3: -fcontracts-prove-shadow switches the proof shadow back on
// after -fno-contracts-prove-shadow, and works with folding switched off.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-explicit.C

// CHECK: "-fcontracts-prove-shadow"

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);
  contract_assert(1 + 1 == 2);
  contract_assert(i < n);       // expected-error {{contract assertion cannot be proven}}
  return i;
}
