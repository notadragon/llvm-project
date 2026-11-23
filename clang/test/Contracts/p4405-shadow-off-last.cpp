// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: -fno-contracts-prove-shadow after -fcontracts-p4405 also
// switches the proof shadow off (p4405-shadow-off.cpp has it first).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-off-last.C

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);     // expected-error {{contract assertion cannot be proven}}
  return i;
}
