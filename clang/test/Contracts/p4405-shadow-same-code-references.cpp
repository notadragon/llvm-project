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

// P4405: a proof shadow's references to variables and functions do not count
// when the optimizers work out their properties for the real code.  bump,
// which only the predicate of f's assertion calls (so only f's shadow), is the
// only code that stores to counter; counter is found never stored to in the
// real module and its load folds away.  Only the predicate of h's assertion
// takes the address of g; g's address is not taken in the real module, so
// dead-argument elimination drops g's unused parameter.
// The real module, its IR and its object code, are exactly what they are
// without the shadows (-fno-contracts-prove-shadow) and with every evaluation
// ignore; -fno-contracts-prove-assume, so that nothing proven is assumed.
// Clang's shadows live in a proof module of their own, so the real code never
// sees them as callers or users.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-references.C
// (GCC-995, which GCC's shadow does change).

extern int sink(int);

static int counter;
static bool bump() { ++counter; return true; }

int f(int n) {
  contract_assert(bump() && n > 0);
  return sink(n) + counter;
}

extern bool ok(int (*)(int, int));

__attribute__((noinline)) static int g(int x, int) { return sink(x) * 2; }

int h(int n) {
  contract_assert(ok(&g));
  return g(n, 1) + g(n + 1, 2);
}

// REMARK-COUNT-2: remark: proof shadow: not proven

// CHECK-NOT: counter
// CHECK: define {{.*}} @_Z1fi(
// CHECK-NOT: counter
// CHECK: define internal {{.*}} @_ZL1gii(i32 noundef %x)
// CHECK-NOT: counter
