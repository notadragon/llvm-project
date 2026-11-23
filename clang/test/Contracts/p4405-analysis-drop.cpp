// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-analysis-drop.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - -Rcontract-proof -verify %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -fno-contracts-prove-assume -fcontract-configuration-file=%S/p4405-analysis-drop.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - -Rcontract-proof -verify=off %s | FileCheck %s --check-prefix=OFF
// REQUIRES: x86-registered-target

// D4405 ("The Analysis Evaluation"): a sound proof of an analysis
// evaluation may drop the runtime evaluations it covers in the same code,
// and with -fcontracts-prove-assume (implied by -fcontracts-p4405) the proof
// is an assumption, so the check that follows it goes away.  In sound(),
// the second contract_assert is analyzed with prove (proven from the first,
// an assume evaluation, which the shadow assumes and Clang's real code
// lowers like ignore, CLANG-111) and checked with enforce: the assumption
// made of the proof, placed ahead of the check, removes it.  A caller's
// proof never removes a callee-side check: kk::k(y), proven at its call,
// keeps its own enforced check.  The second RUN line has
// -fno-contracts-prove-assume: the check stays (gnu_gcc's
// p4405-analysis-drop-off.C).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-drop.C (GCC
// proves from an enforced precondition of an inlined static callee; Clang's
// demonstration is assume then prove, as in p4405-prove-assume.cpp)

int sink(int);

namespace kk {
[[gnu::noinline]] int k(int x) pre(x > 0) // enforce, analysis prove
{
  return sink(x);
}
} // namespace kk

int sound(int y) {
  contract_assert(y > 0); // line 32: assume
  contract_assert(y > 0); // expected-remark {{proof shadow: proven, sound; assumed}}
                          // off-remark@-1 {{proof shadow: proven, sound}}
  return sink(y / 4);
}

int call_k(int y) {
  if (y > 0)
    return kk::k(y); // expected-remark {{proof shadow: proven, sound; assumed}}
                     // off-remark@-1 {{proof shadow: proven, sound}}
  return 0;
}

// CHECK-LABEL: define {{.*}} @_ZN2kk1kEi(
// CHECK: __cxa_contract_violation_pre_enforce_pf
// CHECK-LABEL: define {{.*}} @_Z5soundi(
// CHECK-NOT: __cxa_contract_violation
// CHECK: ret
// CHECK-LABEL: define {{.*}} @_Z6call_ki(
// CHECK-NOT: __cxa_contract_violation
// CHECK: ret

// OFF-LABEL: define {{.*}} @_Z5soundi(
// OFF: __cxa_contract_violation_assert_enforce_pf
// OFF: ret
