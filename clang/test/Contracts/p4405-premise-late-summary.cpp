// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontract-configuration-file=%S/p4405-premise-late-summary.json -emit-llvm -o - -Rcontract-proof -verify %s | FileCheck %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): a replaceable function given an
// assumption by -fcontracts-prove-assume exports nothing it learns from it.  In
// f and fs an earlier quick_enforce lets the shadow prove the diagnose
// evaluation soundly (an own-body premise), and the assumption made of it (and
// the trap itself) tells the optimizers that the result is positive.  For the
// static fs that reaches use_fs, which drops its test of the result; f is
// inline, so its definition is not exact (linkonce_odr) and LLVM infers nothing
// from it for a caller that does not inline it: use_f keeps its test.  Clang
// needs no hook for this (gnu_gcc stops its late value-range, pure-const and
// mod-ref summaries of such a function).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-late-summary.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int bad ();

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // quick_enforce
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return x;
}

int use_f (int y)
{
  int r = f (y);
  if (r <= 0)
    return bad ();
  return r;
}

[[gnu::noinline]] static int fs (int x)
{
  contract_assert (x > 0);  // quick_enforce
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return x;
}

int use_fs (int y)
{
  int r = fs (y);
  if (r <= 0)
    return bad ();
  return r;
}


// Only use_f still calls bad.
// CHECK-LABEL: define {{.*}} @_Z5use_fi(
// CHECK: call {{.*}} @_Z3badv(
// CHECK-LABEL: define {{.*}} @_Z6use_fsi(
// CHECK-NOT: @_Z3badv(
// CHECK: ret i32
