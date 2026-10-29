// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-prove-assume.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - -Rcontract-proof -verify %s | FileCheck %s
// REQUIRES: x86-registered-target

// P4405 -fcontracts-prove-assume (implied by -fcontracts-p4405): a prove or
// diagnose evaluation the proof shadow proves soundly is an assumption in
// the real function, as if evaluated with assume.  The assumption is
// decided before the real function is optimized and placed as
// llvm.assume of the predicate, which is never evaluated: what is left of
// it once the optimizers have used it is deleted.
//
// In Clang the real function gains from this where the shadow knows more
// than the real code: here, after an assume evaluation, which the shadow
// assumes (guaranteed) but the real code does not (Clang lowers assume
// like ignore, CLANG-111).  The proven assertion after it becomes the
// assumption that x is positive, which turns x / 4 into a shift.  After
// observe the proof is only conditional (the handler may return), so
// nothing is assumed and the division stays.  After quick_enforce the
// real code already knows x is positive (its trap does not return), so
// the assumption adds nothing (gnu_gcc's trap is inline too since M5-GCC;
// its sound () proves from a static callee inlined into the shadow only).
// The configuration selects each earlier evaluation's semantic by its
// line; every other evaluation proves.  p4405-prove-assume-off.cpp has
// -fno-contracts-prove-assume.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-assume.C

int sink(int);

int sound(int x) {
  contract_assert(x > 0);       // line 30: assume
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(x / 4);
}

int conditional(int x) {
  contract_assert(x > 0);       // line 36: observe
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, conditional}}
  return sink(x / 4);
}

int quick(int x) {
  contract_assert(x > 0);       // line 42: quick_enforce
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(x / 4);
}

// CHECK-LABEL: define {{.*}} @_Z5soundi(
// CHECK-NOT: sdiv
// CHECK: lshr i32 %{{.*}}, 2
// CHECK-NOT: llvm.assume
// CHECK: ret
// CHECK-LABEL: define {{.*}} @_Z11conditionali(
// CHECK: sdiv i32 %{{.*}}, 4
// CHECK: ret
// CHECK-LABEL: define {{.*}} @_Z5quicki(
// CHECK-NOT: sdiv
// CHECK: lshr i32 %{{.*}}, 2
// CHECK: ret
// CHECK-NOT: __clang_contract
// CHECK-NOT: clang.contract
