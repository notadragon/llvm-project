// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -O2 -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -O2 -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405 (-fcontracts-prove-callee-premises), notadragon_wg21 DECISIONS.md
// E199 (b), E196: a proof that rests on a callee's observe or ignore
// evaluation, through its premise copy, is conditional, so under
// -fcontracts-prove-assume (implied by -fcontracts-p4405) it never becomes
// an assumption.  h's proof relies on idx's postcondition: the real code
// keeps comparing i with 0.  s's proof needs no premise: sound, and assumed.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-callee-premises-not-assumed.C

#include <contracts>
using namespace std::contracts::labels;

int idx(int x) post(r : r == 0) { return x; }

int h(int x) {
  int i = idx(x);
  contract_assert<always_diagnose>(i == 0); // expected-remark {{proof shadow: proven, conditional}}
  return i == 0;
}

int s(int x) {
  if (x != 0)
    __builtin_unreachable();
  int i = idx(x);
  contract_assert<always_diagnose>(i == 0); // expected-remark {{proof shadow: proven, sound; assumed}}
  return i == 0;
}

// CHECK-LABEL: define {{.*}}@_Z1hi(
// CHECK-NOT: ret i32 1
// CHECK: icmp eq i32 %x, 0
// CHECK-NOT: ret i32 1
// CHECK-LABEL: define {{.*}}@_Z1si(
// CHECK: ret i32 1
