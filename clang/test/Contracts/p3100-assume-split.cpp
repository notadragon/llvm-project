// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.assume.false.nonpure.opaque:observe,ub:dcl.attr.assume.false.nonpure.side.effects:observe,ub:dcl.attr.assume.false.pure:quick_enforce \
// RUN:   | FileCheck %s

// P3100: an [[assume]] predicate that cannot be evaluated freely reports
// ub:dcl.attr.assume.false.nonpure.opaque when it might have a side effect
// only through a call (an opaque call, a call to a noreturn function), and
// .nonpure.side.effects when it has one of its own (an increment, an
// assignment, a volatile access).  Both leaves allow only assume and ignore,
// so "observe" for either clamps to ignore: nothing is evaluated and nothing
// is reported.
//
// On Clang the two leaves cannot be told apart in the output: the status-quo
// "assume" emits llvm.assume only for a side-effect-free predicate, so for
// these predicates assume and ignore both emit nothing.  (GCC keeps an
// IFN_ASSUME for them, which its mirrors count.)  The configuration is still
// accepted per leaf.
//
// A predicate that may trap but has no side effect (*p > 0) is checkable on
// Clang, the .pure leaf -- here "quick_enforce", so it traps; GCC reports it
// as .nonpure.opaque (CLANG-732, a divergence).
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-assume-split-opaque.C and
// p3100-assume-split-side-effects.C)

bool opaque(int);
[[noreturn]] bool never();
volatile int vol;

// CHECK-LABEL: define{{.*}} @_Z8f_opaquei(
// CHECK-NOT: call {{.*}}@_Z6opaquei
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: llvm.assume
// CHECK: ret i32
int f_opaque(int x) { [[assume(opaque(x))]]; return x; }

// CHECK-LABEL: define{{.*}} @_Z6f_trapPi(
// CHECK: call void @llvm.trap()
int f_trap(int *p) { [[assume(*p > 0)]]; return *p; }

// CHECK-LABEL: define{{.*}} @_Z10f_noreturni(
// CHECK-NOT: call {{.*}}@_Z5neverv
// CHECK-NOT: __cxa_contract_violation
// CHECK: ret i32
int f_noreturn(int x) { [[assume(x > 0 || never())]]; return x; }

// CHECK-LABEL: define{{.*}} @_Z6f_incri(
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: add nsw
// CHECK: ret i32
int f_incr(int x) { [[assume(++x > 1)]]; return x; }

// CHECK-LABEL: define{{.*}} @_Z8f_assigni(
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: store i32 2
// CHECK: ret i32
int f_assign(int x) { [[assume((x = 2) > 1)]]; return x; }

// CHECK-LABEL: define{{.*}} @_Z10f_volatilei(
// CHECK-NOT: load volatile
// CHECK-NOT: __cxa_contract_violation
// CHECK: ret i32
int f_volatile(int x) { [[assume(vol > x)]]; return x; }
