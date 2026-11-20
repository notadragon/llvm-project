// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2
// DEFINE: %{shadow} = -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven
// RUN: %{cc1} %{shadow} -emit-llvm -o %t.shadow.ll -Rcontract-proof %s 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: %{cc1} %{shadow} -fno-contracts-prove-shadow -emit-llvm -o %t.noshadow.ll %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.shadow.ll %t.noshadow.ll
// RUN: diff %t.shadow.ll %t.ignore.ll
// RUN: FileCheck %s --input-file=%t.shadow.ll
// RUN: %{cc1} %{shadow} -emit-obj -o %t.shadow.o %s
// RUN: %{cc1} %{shadow} -fno-contracts-prove-shadow -emit-obj -o %t.noshadow.o %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ignore.o %s
// RUN: cmp %t.shadow.o %t.noshadow.o
// RUN: cmp %t.shadow.o %t.ignore.o
// REQUIRES: x86-registered-target

// P4405: neither a proof shadow's size nor what is inlined into it counts
// against the real code's inlining.  h is called from six functions, five of
// which hold an unproven evaluation, so five shadows call it too; it is
// inlined into all six real callers and not emitted.  (Clang's inliner has no
// unit-wide growth budget like gnu_gcc's inline-unit-growth; the test pins
// the same outcome.)
// The real module, its IR and its object code, are exactly what they are
// without the shadows (-fno-contracts-prove-shadow) and with every evaluation
// ignore; -fno-contracts-prove-assume, so that nothing proven is assumed.
// Clang's shadows live in a proof module of their own, so the real code never
// sees them as callers or users.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-unit-growth.C
// (GCC-907).

extern int sink(int);

static inline int h(int n) {
  return sink(n) + sink(n + 1) * sink(n + 2) + sink(n + 3);
}

int f1(int n) { contract_assert(n > 1); return h(n) + 1; }
int f2(int n) { contract_assert(n > 2); return h(n) + 2; }
int f3(int n) { contract_assert(n > 3); return h(n) + 3; }
int f4(int n) { contract_assert(n > 4); return h(n) + 4; }
int f5(int n) { contract_assert(n > 5); return h(n) + 5; }
int f6(int n) { return h(n) + 6; }

// REMARK-COUNT-5: remark: proof shadow: not proven

// CHECK-NOT: @_ZL1hi(
