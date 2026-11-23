// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-premise-transitive.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - -Rcontract-proof -verify %s | FileCheck %s
// REQUIRES: x86-registered-target

// P4405 premises decided by this TU (T2-Clang): what the shadow relies on
// through more than one function.  Every assertion but the consumers is
// enforce; the consumers are diagnose.
//
// * through_exact: the shadow calls out of line x_calls, an external
//   function (an exact definition, whose facts LLVM may use), which calls
//   out of line the replaceable h_never holding a premise: conditional,
//   as for a direct call (gnu_gcc's transitive check).
// * x_inlines inlines the replaceable h_auto in the proof module, and the
//   shadow of through_inlining calls x_inlines out of line: what LLVM
//   learns of x_inlines may rest on that inlined copy, so the real
//   x_inlines is made to inline h_auto too, and the proof is sound.
// * chain: the shadow inlines mid, a replaceable function holding no
//   premise of its own, and through it h_chain, which holds one: both
//   calls are made always_inline in the real code (mid's in its body).
// * static_chain: the same through mid_static, a static function, whose
//   copy is this TU's: only the call of h_chain2 in it must be inlined.
//
// The forced calls may well have been inlined anyway; that the real code
// does what the proof relies on is what the checks show
// (p4405-premise-shadow-only.cpp provokes a call it would not inline).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-transitive.C
// (same outcomes; GCC checks the inlining its real code made instead).

int sink(int);

[[gnu::noinline]] inline void h_never(int x) pre(x > 0) { sink(x); }
[[gnu::noinline]] void x_calls(int y) { h_never(y); }

int through_exact(int x, int y) {
  x_calls(y);
  if (x > 0)
    contract_assert(x > 0);     // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

inline int h_auto(int x) pre(x > 0) { return sink(x); }
[[gnu::noinline]] int x_inlines(int y) { return h_auto(y) + 1; }

int through_inlining(int x, int y) {
  x_inlines(y);
  if (x > 0)
    contract_assert(x > 0);     // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

inline int h_chain(int x) pre(x > 0) { return sink(x); }
inline int mid(int x) { return h_chain(x) + sink(x + 1); }

int chain(int y) {
  int r = mid(y);
  contract_assert(y > 0);       // expected-remark {{proof shadow: proven, sound; assumed}}
  return r;
}

inline int h_chain2(int x) pre(x > 0) { return sink(x); }
static int mid_static(int x) { return h_chain2(x) + sink(x + 1); }

int static_chain(int y) {
  int r = mid_static(y);
  contract_assert(y > 0);       // expected-remark {{proof shadow: proven, sound; assumed}}
  return r;
}

// CHECK-LABEL: define {{.*}} @_Z9x_inlinesi(
// CHECK-NOT: call {{.*}} @_Z6h_autoi(
// CHECK: ret i32
// CHECK-LABEL: define {{.*}} @_Z5chaini(
// CHECK-NOT: call {{.*}} @_Z3midi(
// CHECK-NOT: call {{.*}} @_Z7h_chaini(
// CHECK: ret i32
// CHECK-LABEL: define {{.*}} @_Z12static_chaini(
// CHECK-NOT: call {{.*}} @_Z8h_chain2i(
// CHECK: ret i32
