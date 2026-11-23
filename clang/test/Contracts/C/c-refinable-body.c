// RUN: %clang_cc1 -fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fno-inline -emit-llvm -o - %s | FileCheck %s
// REQUIRES: x86-registered-target
//
// The refinable-body rule in C (P4299): a C99 inline definition, or a GNU
// extern inline one, is not the definition a call that is not inlined
// reaches -- that is the external definition in another translation unit,
// which may evaluate the function's contract assertions with another
// semantic (C has no labels, so no evaluation is settled) -- so its body
// (available_externally) must not make it nounwind (CLANG-824: it did, and
// a caller's cleanup was dropped) nor give its callers a return value
// range.  Every check traps here, so no body can throw: callers of the
// inline definitions keep their cleanups on the exceptional path (the
// cleanup_fn calls in landing pads) and their comparisons; the static and
// the external definitions are the only definitions of their symbols, so
// their callers drop both; inside the function the trap still folds the
// comparison after the check.  (Runnable/ignore-ipa-replaceable-c99.cpp
// runs the case across translation units.)
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-refinable-body.c

#include <contracts.h>

void cleanup_fn (int *p);

inline int f_c99 (int x) { _ContractAssert (x > 0); return x; }
extern inline __attribute__ ((gnu_inline)) int
f_gnu (int x) { _ContractAssert (x > 0); return x; }
inline int f_pre (int x) _Pre (x > 0) { return x; }
inline int f_post (int x) _Post (r: r > 0) { return x; }
static int f_s (int x) { _ContractAssert (x > 0); return x; }
int f_e (int x) { _ContractAssert (x > 0); return x; }
int h_e (int x) { _ContractAssert (x > 0); return x > 0 ? 1234 : 5678; }

int use_c99 (int y) { int c99 __attribute__ ((cleanup (cleanup_fn))) = 0; return f_c99 (y) > 0; }
int use_gnu (int y) { int gnu __attribute__ ((cleanup (cleanup_fn))) = 0; return f_gnu (y) > 0; }
int use_pre (int y) { int pre __attribute__ ((cleanup (cleanup_fn))) = 0; return f_pre (y) > 0; }
int use_post (int y) { int post __attribute__ ((cleanup (cleanup_fn))) = 0; return f_post (y) > 0; }
int use_s (int y) { int s __attribute__ ((cleanup (cleanup_fn))) = 0; return f_s (y) > 0; }
int use_e (int y) { int e __attribute__ ((cleanup (cleanup_fn))) = 0; return f_e (y) > 0; }

// CHECK-LABEL: define {{.*}}@h_e(
// CHECK-NOT: 5678
// CHECK: ret i32 1234
// CHECK-LABEL: define {{.*}}@use_c99(
// CHECK: %[[R1:.*]] = invoke {{.*}}@f_c99(
// CHECK: icmp sgt i32 %[[R1]], 0
// CHECK: landingpad
// CHECK-NEXT: cleanup
// CHECK-NEXT: {{call|invoke}} void @cleanup_fn(
// CHECK-LABEL: define {{.*}}@use_gnu(
// CHECK: %[[R2:.*]] = invoke {{.*}}@f_gnu(
// CHECK: icmp sgt i32 %[[R2]], 0
// CHECK: landingpad
// CHECK-LABEL: define {{.*}}@use_pre(
// CHECK: %[[R3:.*]] = invoke {{.*}}@f_pre(
// CHECK: icmp sgt i32 %[[R3]], 0
// CHECK: landingpad
// CHECK-LABEL: define {{.*}}@use_post(
// CHECK: %[[R4:.*]] = invoke {{.*}}@f_post(
// CHECK: icmp sgt i32 %[[R4]], 0
// CHECK: landingpad
// CHECK-LABEL: define {{.*}}@use_s(
// CHECK-NOT: landingpad
// CHECK: ret i32 1
// CHECK-LABEL: define {{.*}}@use_e(
// CHECK-NOT: landingpad
// CHECK: ret i32 1
