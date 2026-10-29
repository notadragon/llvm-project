// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ACC

// P3100: under quick_enforce, a call through `*fp` (also through
// `static_cast<F &>` or a conditional) and a reference bound to `*fp` get
// one null guard each, under the .indirection leaf (CLANG-755, CLANG-757); a
// function named directly (`(*fn)()`, `F &r = fn`), a reference to a
// function and a plain call through the pointer, `fp()`, which names no
// indirection, get none.  .access alone guards none of them.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-function-indirection-codegen.C
// and its -access variant.)

using F = int();
int fn();

// CHECK-LABEL: define{{.*}} @_Z4callPFivE(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_Z4callPFivE(
// ACC-NOT: llvm.trap
// ACC: ret i32
int call(F *fp) { return (*fp)(); }

// CHECK-LABEL: define{{.*}} @_Z9call_castPFivE(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_Z9call_castPFivE(
// ACC-NOT: llvm.trap
// ACC: ret i32
int call_cast(F *fp) { return static_cast<F &>(*fp)(); }

// CHECK-LABEL: define{{.*}} @_Z9call_condPFivES0_b(
// CHECK: phi ptr
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret i32
int call_cond(F *fp, F *gp, bool b) { return (b ? *fp : *gp)(); }

// CHECK-LABEL: define{{.*}} @_Z4bindPFivE(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z4bindPFivE(
// ACC-NOT: llvm.trap
// ACC: ret ptr
F &bind(F *fp) { return *fp; }

// CHECK-LABEL: define{{.*}} @_Z10call_namedv(
// CHECK-NOT: llvm.trap
// CHECK: ret i32
int call_named() { return (*fn)(); }

// CHECK-LABEL: define{{.*}} @_Z10bind_namedv(
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
F &bind_named() { return fn; }

// CHECK-LABEL: define{{.*}} @_Z8bind_refRFivE(
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
F &bind_ref(F &r) { return r; }

// CHECK-LABEL: define{{.*}} @_Z10call_plainPFivE(
// CHECK-NOT: llvm.trap
// CHECK: ret i32
int call_plain(F *fp) { return fp(); }
