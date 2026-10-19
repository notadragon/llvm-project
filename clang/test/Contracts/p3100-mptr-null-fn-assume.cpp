// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:ignore \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=QUICK

// P3100: a call through a null pointer to member function
// (ub:expr.mptr.oper.member.func.null.function) left at the default "assume",
// or configured to "ignore": no check is emitted (the operation itself is not
// run here); "quick_enforce" emits one trap.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-mptr-null-fn-assume.C and
// p3100-mptr-null-fn-ignore.C)

struct S {
  int f() { return 1; }
  virtual int g() { return 2; }
};
// CHECK-LABEL: define{{.*}} @_Z4callR1SMS_FivE(
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: call void @llvm.trap()
// CHECK: ret i32
// QUICK-LABEL: define{{.*}} @_Z4callR1SMS_FivE(
// QUICK: call void @llvm.trap()
// QUICK-NOT: call void @llvm.trap()
// QUICK: ret i32
int call(S &s, int (S::*pm)()) { return (s.*pm)(); }
