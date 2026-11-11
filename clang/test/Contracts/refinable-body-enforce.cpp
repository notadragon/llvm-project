// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=enforce -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=quick_enforce -emit-llvm -o - %s | FileCheck %s
//
// Direction A of the refinable-body design, which LLVM already gets right:
// a contract check lets the code after it rely on its predicate, but a
// function another translation unit may also define (here an inline
// function, linkonce_odr) may evaluate the assertion differently there, for
// example ignore it, so callers that do not inline it must not take its
// return value from this body.  LLVM derives nothing from an inexact
// definition (FunctionAttrs, IPSCCP, GlobalsModRef), so use_i keeps its
// comparison; the static and the external non-inline function have no
// other definition, so their callers fold it; inside the inline function
// the check still folds the comparison after it, and an always_inline
// callee's check folds the caller's.  (Runnable/enforce-ipa-replaceable.cpp
// runs the first case across two translation units.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-2.C and
// refinable-body-quick.C

[[gnu::noinline]] inline int fi (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] static int fs (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] int fe (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] inline int hi (int x)
{
  contract_assert (x > 0);
  return x > 0 ? 1234 : 5678;
}

[[gnu::always_inline]] inline int ai (int x) { contract_assert (x > 0); return x; }

int use_i (int y) { return fi (y) > 0; }
int use_s (int y) { return fs (y) > 0; }
int use_e (int y) { return fe (y) > 0; }
int use_h (int y) { return hi (y); }
int use_a (int y) { return ai (y) > 0 ? 4321 : 8765; }

// CHECK-LABEL: define {{.*}}@_Z5use_ii(
// CHECK: %[[R:.*]] = tail call {{.*}}@_Z2fii(
// CHECK-NEXT: icmp sgt i32 %[[R]], 0
// CHECK-LABEL: define {{.*}}@_Z5use_si(
// CHECK: call {{.*}}@_ZL2fsi(
// CHECK-NEXT: ret i32 1
// CHECK-LABEL: define {{.*}}@_Z5use_ei(
// CHECK: call {{.*}}@_Z2fei(
// CHECK-NEXT: ret i32 1
// CHECK-LABEL: define linkonce_odr {{.*}}@_Z2hii(
// CHECK-NOT: 5678
// CHECK: ret i32 1234
// CHECK-LABEL: define {{.*}}@_Z5use_ai(
// CHECK-NOT: 8765
// CHECK: ret i32 4321
