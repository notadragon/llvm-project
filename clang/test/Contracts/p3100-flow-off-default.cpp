// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts -fcontracts-p3100 -Wno-return-type -O2 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-flow-off-enforce.json -Wno-return-type -O2 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s --check-prefix=ENFORCE

// P3100: with -fcontracts-p3100 and no configuration, the flow-off-end implicit
// assertion defaults to "assume" -- today's behavior: no check is emitted and
// the flow-off path is left unreachable.  The ENFORCE run is the control: the
// same function under an enforce configuration does call the violation entry
// point, so the absence of one in the default run means something.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-flow-off-default.C)

int classify(int x) {
  if (x > 0)
    return x * 2;
  // x <= 0: control flows off the end (assume: left as UB, no check).
}

// CHECK-LABEL: define {{.*}} @_Z8classifyi(
// CHECK-NOT:   call {{.*}}@__cxa_contract_violation
// CHECK:       unreachable
// CHECK-NOT:   call {{.*}}@__cxa_contract_violation
// CHECK:       {{^}}}

// ENFORCE-LABEL: define {{.*}} @_Z8classifyi(
// ENFORCE:       call void @__cxa_contract_violation_implicit_enforce_pf(
// ENFORCE-NEXT:  unreachable
