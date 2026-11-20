// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:quick_enforce \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:quick_enforce \
// RUN:   | FileCheck %s

// P3100: one null dereference through o->*pm is checked once -- on o, not
// again on the member's address o + pm or on a reference bound to it --
// also when the null member pointer check
// (ub:expr.mptr.oper.member.func.null.data) guards the member's address,
// and for a member of a class member at a nonzero offset, o->s.*pm, whose
// check is on o too.  (The run-time counts are in
// Runnable/p3100-null-member-subobject-once.cpp.)
// (GCC mirror:
// g++.dg/contracts/cpp26/p3100-null-member-subobject-once-guarded.C.)

struct M { int m; int n; };
struct T { int t; M s; };

// CHECK-LABEL: define{{.*}} @_Z2rdP1MMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o1.addr
// CHECK-NEXT: icmp eq ptr [[P]], null
// CHECK-NOT: icmp eq ptr
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK-NOT: icmp eq ptr
// CHECK: ret i32
int rd(M *o1, int M::*pm) { return o1->*pm; }

// CHECK-LABEL: define{{.*}} @_Z3refP1MMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o2.addr
// CHECK-NEXT: icmp eq ptr [[P]], null
// CHECK-NOT: icmp eq ptr
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK-NOT: icmp eq ptr
// CHECK: ret ptr
int &ref(M *o2, int M::*pm) { return o2->*pm; }

// CHECK-LABEL: define{{.*}} @_Z4rd_sP1TM1Mi(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o3.addr
// CHECK-NEXT: icmp eq ptr [[P]], null
// CHECK-NOT: icmp eq ptr
// CHECK: icmp ne i64 {{%[0-9]+}}, -1
// CHECK-NOT: icmp eq ptr
// CHECK: ret i32
int rd_s(T *o3, int M::*pm) { return o3->s.*pm; }
