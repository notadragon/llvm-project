// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-prove-assume-off.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - -Rcontract-proof -verify %s | FileCheck %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fno-contracts-prove-assume -c %s 2>&1 | FileCheck --check-prefix=DRIVER %s
// REQUIRES: x86-registered-target

// P4405: with -fno-contracts-prove-assume a soundly proven evaluation is not
// an assumption: the division p4405-prove-assume.cpp turns into a shift
// stays, and nothing in the real function remains of the proof.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-assume-off.C

// DRIVER: "-fno-contracts-prove-assume"

int sink(int);

int sound(int x) {
  contract_assert(x > 0);       // line 16: assume
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound}}
  return sink(x / 4);
}

int conditional(int x) {
  contract_assert(x > 0);       // line 22: observe
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, conditional}}
  return sink(x / 4);
}

// CHECK-LABEL: define {{.*}} @_Z5soundi(
// CHECK: sdiv i32 %{{.*}}, 4
// CHECK-NOT: llvm.assume
// CHECK: ret
// CHECK-LABEL: define {{.*}} @_Z11conditionali(
// CHECK: sdiv i32 %{{.*}}, 4
// CHECK-NOT: __clang_contract
// CHECK-NOT: clang.contract
