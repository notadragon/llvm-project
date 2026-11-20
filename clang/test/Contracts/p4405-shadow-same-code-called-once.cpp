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

// P4405: a proof shadow is not a caller of the real code, where the inliner's
// view of the callers decides.  big is called once by real code, from f, which
// holds an unproven evaluation (its shadow calls big too); big2 is called once
// by the real code of g and once more by g's assertion, which only the shadow
// evaluates; big3, an inline (linkonce_odr) function, is called once from h.
// big and big2 are inlined into their real callers and not emitted, as when
// called once.
// The real module, its IR and its object code, are exactly what they are
// without the shadows (-fno-contracts-prove-shadow) and with every evaluation
// ignore; -fno-contracts-prove-assume, so that nothing proven is assumed.
// Clang's shadows live in a proof module of their own, so the real code never
// sees them as callers or users.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-called-once.C
// (GCC-907); see also p4405-shadow-real-inlining.cpp.

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

static int big2(int n) {
  int s = 0;
  for (int i = 0; i < n; ++i) {
    s += sink(i) * 3;
    s ^= sink(s);
    s += sink(s + i);
  }
  if (s > 100)
    s = sink(s) + sink(s + 1) + sink(s + 2) + sink(s + 3);
  return s;
}

int g(int n) {
  contract_assert(big2(n + 1) > 0);
  return big2(n) + 1;
}

inline int big3(int n) {
  int s = 0;
  for (int i = 0; i < n; ++i) {
    s += sink(i) * 3;
    s ^= sink(s);
    s += sink(s + i);
  }
  for (int i = 0; i < n; ++i)
    s ^= sink(s - 1);
  if (s > 100)
    s = sink(s) + sink(s + 1) + sink(s + 2) + sink(s + 3);
  return s;
}

int h(int n) {
  contract_assert(n != 3);
  return big3(n) - 1;
}

// REMARK-COUNT-3: remark: proof shadow: not proven

// CHECK-NOT: @_ZL{{3bigi|4big2i}}(
// CHECK: define {{.*}} @_Z1fi(
// CHECK-NOT: @_ZL{{3bigi|4big2i}}(
