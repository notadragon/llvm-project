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

// P4405: interprocedural constant propagation does not count the calls a
// proof shadow makes.  g is called with the constant 7 by the real code, and
// with a variable by f's assertion, which only f's shadow evaluates; IPSCCP
// propagates 7 into g and dead-argument elimination drops the parameter, as
// without the shadow.
// The real module, its IR and its object code, are exactly what they are
// without the shadows (-fno-contracts-prove-shadow) and with every evaluation
// ignore; -fno-contracts-prove-assume, so that nothing proven is assumed.
// Clang's shadows live in a proof module of their own, so the real code never
// sees them as callers or users.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-ipa-cp.C
// (GCC-907).

extern int sink(int);

__attribute__((noinline)) static int g(int x, int k) {
  int s = 0;
  for (int i = 0; i < k; ++i)
    s += sink(x + i);
  return s;
}

int f(int n) {
  contract_assert(g(n, n) >= 0);
  return g(n, 7) + 1;
}

int h(int n) { return g(n + 1, 7); }

// REMARK-COUNT-1: remark: proof shadow: not proven

// CHECK: define internal {{.*}} @_ZL1gii(i32 noundef %x)
