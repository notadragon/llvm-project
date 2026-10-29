// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:quick_enforce \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:quick_enforce \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:ignore \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:ignore \
// RUN:   | FileCheck %s --check-prefix=IGNORE

// P3100: with the known-bound checks (ub:expr.add.out.of.bounds.known) and
// the past-the-end indirection check (ub:expr.unary.dereference.invalid.known)
// configured, nested arithmetic, an indirection through it and a subscript of
// it or of the array itself at a constant position within the array emit no
// check -- no never-taken guard on a constant false at -O0, and under
// "ignore" no substituting select either; the same arithmetic at a run-time
// position does.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-nested-const.C)
int a[4];

// CHECK-LABEL: define{{.*}} @_Z6nestedv(
// IGNORE-LABEL: define{{.*}} @_Z6nestedv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: call void @llvm.trap()
// CHECK: ret ptr
int *nested() { return (a + 1) + 3; }

// CHECK-LABEL: define{{.*}} @_Z5derefv(
// IGNORE-LABEL: define{{.*}} @_Z5derefv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: call void @llvm.trap()
// CHECK: ret i32
int deref() { return *(a + 1 + 2); }

// CHECK-LABEL: define{{.*}} @_Z6directv(
// IGNORE-LABEL: define{{.*}} @_Z6directv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: call void @llvm.trap()
// CHECK: ret i32
int direct() { return *(a + 3); }

// CHECK-LABEL: define{{.*}} @_Z3subv(
// IGNORE-LABEL: define{{.*}} @_Z3subv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: br i1
// CHECK: ret i32
int sub() { return (a + 1)[2]; }

// CHECK-LABEL: define{{.*}} @_Z7subaddrv(
// IGNORE-LABEL: define{{.*}} @_Z7subaddrv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: br i1
// CHECK: ret ptr
int *subaddr() { return &(a + 1)[2]; }

// CHECK-LABEL: define{{.*}} @_Z5plainv(
// IGNORE-LABEL: define{{.*}} @_Z5plainv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: br i1
// CHECK: ret i32
int plain() { return a[2]; }

// CHECK-LABEL: define{{.*}} @_Z9plainaddrv(
// IGNORE-LABEL: define{{.*}} @_Z9plainaddrv(
// IGNORE-NOT: ub.subst
// IGNORE: ret
// CHECK-NOT: br i1
// CHECK: ret ptr
int *plainaddr() { return &a[2]; }

// CHECK-LABEL: define{{.*}} @_Z10nested_vari(
// CHECK: call void @llvm.trap()
// CHECK: ret ptr
int *nested_var(int j) { return (a + 1) + j; }
