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

// P4405: interprocedural argument and return-value optimizations do not count
// the calls a proof shadow makes.  The real code ignores g's result, which f's
// assertion, evaluated only in f's shadow, uses; dead-argument elimination
// makes g return nothing, as without the shadow.
// The real module, its IR and its object code, are exactly what they are
// without the shadows (-fno-contracts-prove-shadow) and with every evaluation
// ignore; -fno-contracts-prove-assume, so that nothing proven is assumed.
// Clang's shadows live in a proof module of their own, so the real code never
// sees them as callers or users.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-ipa-sra.C
// (GCC-907).

extern int sink(int);

struct S { int a, b, c, d; };

__attribute__((noinline)) static int g(S *p) {
  sink(p->a);
  sink(p->b);
  return p->c;
}

int f(int n) {
  contract_assert(g(nullptr) >= 0);
  S s = {n, n + 1, n + 2, n + 3};
  g(&s);
  return n;
}

int h(int n) {
  S s = {n, 1, 2, 3};
  g(&s);
  return 0;
}

// REMARK-COUNT-1: remark: proof shadow: not proven

// CHECK: define internal fastcc void @_ZL1gP1S(
