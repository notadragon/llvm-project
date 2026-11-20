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

// P4405: identical-function merging merges nothing with code only a proof
// shadow needs.  check, which only f's assertion calls (so only f's shadow),
// has the body of r; r is emitted as itself and check not at all, as without
// the shadow.  (Clang merges identical functions only under -fmerge-functions:
// p4405-shadow-merge-functions.cpp.)
// The real module, its IR and its object code, are exactly what they are
// without the shadows (-fno-contracts-prove-shadow) and with every evaluation
// ignore; -fno-contracts-prove-assume, so that nothing proven is assumed.
// Clang's shadows live in a proof module of their own, so the real code never
// sees them as callers or users.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-icf.C
// (GCC-907).

extern int sink(int);

__attribute__((noinline)) static int check(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

__attribute__((noinline)) int r(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

int f(int n) {
  contract_assert(check(n) > 0);
  return r(n);
}

// REMARK-COUNT-1: remark: proof shadow: not proven

// CHECK-NOT: @_ZL5checki
// CHECK: define {{.*}} @_Z1ri(
// CHECK-NOT: @_ZL5checki
