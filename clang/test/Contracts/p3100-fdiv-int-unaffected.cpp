// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:quick_enforce \
// RUN:   | FileCheck %s

// P3100: configuring only the floating-point leaf
// ub:expr.mul.div.by.zero.fp.div leaves integer division by zero
// (ub:expr.mul.div.by.zero.int) at the default "assume": no integer guard is
// emitted, while the floating-point division gets its trap.  A divisor that
// is a nonzero constant is not guarded.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fdiv-int-unaffected.C)

// CHECK-LABEL: define{{.*}} @_Z4idivii(
// CHECK-NOT: call void @llvm.trap()
// CHECK: sdiv i32
int idiv(int a, int b) { return a / b; }

// CHECK-LABEL: define{{.*}} @_Z4fdivdd(
// CHECK: fcmp oeq double
// CHECK: call void @llvm.trap()
// CHECK: fdiv double
double fdiv(double a, double b) { return a / b; }

// CHECK-LABEL: define{{.*}} @_Z5fhalfd(
// CHECK-NOT: call void @llvm.trap()
// CHECK: fdiv double
double fhalf(double a) { return a / 2.0; }
