// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fcxx-exceptions -fexceptions -Wno-invalid-noreturn \
// RUN:   -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fcxx-exceptions -fexceptions -Wno-invalid-noreturn \
// RUN:   -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:ignore \
// RUN:   | FileCheck %s

// P3100: a [[noreturn]] function that returns
// (ub:dcl.attr.noreturn.eventually.returns) left at the default "assume", or
// configured to "ignore": no check is emitted (the operation itself is not run
// here).
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-noreturn-assume.C and
// p3100-noreturn-ignore.C)

// CHECK-LABEL: define{{.*}} @_Z2nri(
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: call void @llvm.trap()
// CHECK-NOT: @abort
// CHECK: ret void
[[noreturn]] void nr(int x) {
  if (x < 0)
    throw x;
}
