// RUN: %clang_cc1 -std=c++26 -fno-contracts-prove-shadow -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang -### -std=c++26 -fno-contracts-prove-shadow -fcontracts-p4405 -c %s 2>&1 | FileCheck %s
// REQUIRES: x86-registered-target

// P4405 stage 3: -fno-contracts-prove-shadow switches the proof shadow
// off, on either side of -fcontracts-p4405 (p4405-shadow-off-last.cpp has
// it after), so at -O2 only folding runs and the evaluations
// p4405-shadow-prove.cpp proves are diagnosed again, each once.  The
// driver passes the flag on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-off.C

// CHECK: "-fno-contracts-prove-shadow"

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);     // expected-error {{contract assertion cannot be proven}}
  int c = n < 0 ? 0 : 1;
  contract_assert(c >= 0);      // expected-error {{contract assertion cannot be proven}}
  return c;
}
