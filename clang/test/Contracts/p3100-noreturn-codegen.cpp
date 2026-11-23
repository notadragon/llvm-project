// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fcxx-exceptions -fexceptions -Wno-invalid-noreturn \
// RUN:   -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:quick_enforce \
// RUN:   | FileCheck %s

// P3100: the [[noreturn]] check is made once per function, at its normal
// exits only: "quick_enforce" adds one trap to a noreturn function that can
// return, by a return statement or by flowing off its end, none to one that
// cannot (every path throws), none to other functions, and none at a call.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noreturn-codegen.C)

// CHECK-LABEL: define{{.*}} @_Z1ai(
// CHECK: call void @llvm.trap()
// CHECK-NOT: call void @llvm.trap()
// CHECK-LABEL: define{{.*}} @_Z1bi(
// CHECK-NOT: call void @llvm.trap()
// CHECK-LABEL: define{{.*}} @_Z1ci(
// CHECK-NOT: call void @llvm.trap()
// CHECK-LABEL: define{{.*}} @_Z1ei(
// CHECK: call void @llvm.trap()
// CHECK-NOT: call void @llvm.trap()
// CHECK-LABEL: define{{.*}} @_Z1fv(
// CHECK-NOT: call void @llvm.trap()
// CHECK: unreachable
[[noreturn]] void a(int x) {
  if (x)
    throw x;
}
[[noreturn]] void b(int x) {
  if (x)
    throw x;
  throw 0;
}
void c(int x) {
  if (x)
    throw x;
}
[[noreturn]] void d(int);
[[noreturn]] void e(int x) {
  if (x)
    return;
  throw x;
}
void f() { d(1); }
