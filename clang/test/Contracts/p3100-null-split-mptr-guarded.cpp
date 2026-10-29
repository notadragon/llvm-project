// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:quick_enforce \
// RUN:   | FileCheck %s

// P3100: the null check of the object pointer of a pointer-to-data-member
// access whose member is read is the .access leaf of
// ub:expr.unary.dereference.nullptr (see p3100-null-split-mptr.cpp) also
// when the null member pointer check
// (ub:expr.mptr.oper.member.func.null.data) guards the member's address, in
// a function template's instantiation and in a lambda; the address shape
// keeps the .indirection leaf, so only the member pointer is checked there.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-split-mptr-guarded.C.)

struct S { int m; int n; };

// CHECK-LABEL: define{{.*}} @_Z4readP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o1.addr
// CHECK-NEXT: icmp eq ptr [[P]], null
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK: ret i32
int read(S *o1, int S::*pm) { return o1->*pm; }

template <typename U> int tread(U *o2, int U::*pm) { return o2->*pm; }
template int tread<S>(S *, int S::*);

// CHECK-LABEL: define{{.*}} @_Z5treadI1SEiPT_MS1_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o2.addr
// CHECK-NEXT: icmp eq ptr [[P]], null
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK: ret i32

int lread(S *p, int S::*pm) {
  return [](S *o3, int S::*q) { return o3->*q; }(p, pm);
}

// CHECK-LABEL: define{{.*}} @"_ZZ5lreadP1SMS_iENK3$_0clES0_S1_"(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o3.addr
// CHECK-NEXT: icmp eq ptr [[P]], null
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK: ret i32

// CHECK-LABEL: define{{.*}} @_Z4addrP1SMS_i(
// CHECK-NOT: icmp eq ptr {{%[0-9]+}}, null
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK-NOT: icmp eq ptr {{%[0-9]+}}, null
// CHECK: ret ptr
int *addr(S *a1, int S::*pm) { return &(a1->*pm); }
