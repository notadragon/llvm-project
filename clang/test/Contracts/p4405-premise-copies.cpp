// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontract-configuration-file=%S/p4405-premise-copies.json -emit-llvm -o - -Rcontract-proof -verify %s | FileCheck %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): each copy of the real function.
// g's shadow inlines h, so g's diagnose evaluation relying on h's enforce
// precondition is sound and assumed, and g's call of h is made always_inline.
// The attribute is on the call, so every copy of g made by inlining it (into k
// here, or anywhere) inlines h as well: no copy calls h out of line, and
// gnu_gcc's per-copy check (an early inlining that keeps a call it does not tie
// to the obligation) has nothing to catch in Clang.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-copies.C (gnu_gcc,
// whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);
int big (int);

inline int h (int x) pre (x > 0)
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x) + sink (i + x) + sink (i - x);
  return s + sink (x) + sink (x + 1) + sink (x + 2);
}

int g (int y)
{
  int r = h (y);
  contract_assert (y > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return r + (y > 0 ? 1 : 0);
}

int k (int a, int y)
{
  if (a)
    big (1);
  return g (y);
}


// CHECK-NOT: call {{.*}} @_Z1hi(
// CHECK-LABEL: define {{.*}} @_Z1gi(
// CHECK-NOT: call {{.*}} @_Z1hi(
// CHECK-LABEL: define {{.*}} @_Z1kii(
// CHECK-NOT: call {{.*}} @_Z1hi(
