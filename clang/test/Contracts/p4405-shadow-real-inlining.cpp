// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.shadow.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.assume.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.noshadow.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.shadow.ll %t.noshadow.ll
// RUN: diff %t.shadow.ll %t.ignore.ll
// RUN: diff %t.assume.ll %t.ignore.ll
// RUN: FileCheck %s --input-file=%t.shadow.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.shadow.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.ignore.o %s
// RUN: cmp %t.shadow.o %t.ignore.o
// REQUIRES: x86-registered-target

// P4405: a proof shadow does not change how the real code is inlined.  The
// large static big is called once, from f, which holds an unproven diagnose
// evaluation.  With ignore, or with -fno-contracts-prove-shadow, big is
// inlined into f and disappears; with the shadow it must be too, with or
// without -fcontracts-prove-assume (nothing here is proven, so nothing is
// assumed).  Clang's shadows live in a proof module of their own, so the
// real code never sees them as callers.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-real-inlining.C
// (GCC-907, which GCC's shadow does change).

extern int sink(int);

static int big(int n) {
  int s = 0;
  for (int i = 0; i < n; ++i) {
    s += sink(i) * 3;
    s ^= sink(s);
    s += sink(s + i);
  }
  for (int i = 0; i < n; ++i) {
    s += sink(i + 7) * 5;
    s ^= sink(s - 1);
  }
  if (s > 100)
    s = sink(s) + sink(s + 1) + sink(s + 2) + sink(s + 3);
  return s;
}

int f(int n) {
  int r = big(n);
  contract_assert(n > 0);
  return r + n / 4;
}

// big is neither defined nor called; only labels of its inlined body remain.
// CHECK-NOT: @_ZL3bigi(
// CHECK: define {{.*}} @_Z1fi(
// CHECK-NOT: @_ZL3bigi(
