// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce \
// RUN:   | FileCheck %s --check-prefixes=CHECK,ACCESS
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce \
// RUN:   | FileCheck %s --check-prefixes=CHECK,INDIR

// P3100: a pointer-to-data-member access o->*pm whose member is read or
// written is followed by a load or store, so the null check of its object
// pointer `o` is the .access leaf of ub:expr.unary.dereference.nullptr, as
// for o->m; where only the member's address is formed, a reference is bound
// to it, or it is discarded, it is the .indirection leaf.  Configuring only
// .access checks `o` itself in each accessing shape -- a read, a store, a
// compound assignment, an increment, a read of an array member's element
// (CLANG-741), a read of a class member's member, a member of a base class
// at a nonzero offset and of a virtual base (checked after the conversion to
// the base, which keeps a null pointer null) -- and nothing in the address
// shapes; configuring only .indirection checks `a` in each address shape and
// nothing in the accessing ones.  (The check is on `o` itself: the loaded
// address o + pm is not null for a null `o` and a member at a nonzero
// offset.)
// The object expression of a `.*` access, (*o).*pm, is checked the same
// way (CLANG-743), and so is a member of a class member at a nonzero
// offset, o->s.*pm and &(a->s.*pm), whose check is on o (a) too.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-null-split-mptr-access.C and
// p3100-null-split-mptr-indirection.C.)

struct S { int m; int n; int arr[4]; };
struct T { int t; S s; };
struct B0 { int z; };
struct D : B0, S { };
struct V : virtual S { };

// CHECK-LABEL: define{{.*}} @_Z4readP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o1.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int read(S *o1, int S::*pm) { return o1->*pm; }

// CHECK-LABEL: define{{.*}} @_Z5storeP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o2.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret void
void store(S *o2, int S::*pm) { o2->*pm = 1; }

// CHECK-LABEL: define{{.*}} @_Z8compoundP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o3.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret void
void compound(S *o3, int S::*pm) { o3->*pm += 1; }

// CHECK-LABEL: define{{.*}} @_Z4incrP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o4.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret void
void incr(S *o4, int S::*pm) { ++(o4->*pm); }

// CHECK-LABEL: define{{.*}} @_Z3arrP1SMS_A4_ii(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o5.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int arr(S *o5, int (S::*pa)[4], int i) { return (o5->*pa)[i]; }

// CHECK-LABEL: define{{.*}} @_Z5chainP1TMS_1S(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o6.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int chain(T *o6, S T::*ps) { return (o6->*ps).m; }

// CHECK-LABEL: define{{.*}} @_Z4baseP1DM1Si(
// ACCESS: icmp eq ptr %cast.result, null
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int base(D *o7, int S::*pm) { return o7->*pm; }

// CHECK-LABEL: define{{.*}} @_Z5vbaseP1VM1Si(
// ACCESS: icmp eq ptr %cast.result, null
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int vbase(V *o8, int S::*pm) { return o8->*pm; }

// CHECK-LABEL: define{{.*}} @_Z5dreadP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o9.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int dread(S *o9, int S::*pm) { return (*o9).*pm; }

// CHECK-LABEL: define{{.*}} @_Z6memberP1TM1Si(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %o10.addr
// ACCESS-NEXT: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// CHECK: ret i32
int member(T *o10, int S::*pm) { return o10->s.*pm; }

// CHECK-LABEL: define{{.*}} @_Z4addrP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a1.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret ptr
int *addr(S *a1, int S::*pm) { return &(a1->*pm); }

// CHECK-LABEL: define{{.*}} @_Z8arr_addrP1SMS_A4_ii(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a2.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret ptr
int *arr_addr(S *a2, int (S::*pa)[4], int i) { return &(a2->*pa)[i]; }

// CHECK-LABEL: define{{.*}} @_Z10chain_addrP1TMS_1S(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a3.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret ptr
int *chain_addr(T *a3, S T::*ps) { return &(a3->*ps).m; }

// CHECK-LABEL: define{{.*}} @_Z3refP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a4.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret ptr
int &ref(S *a4, int S::*pm) { return a4->*pm; }

// CHECK-LABEL: define{{.*}} @_Z7discardP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a5.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret void
void discard(S *a5, int S::*pm) { a5->*pm; }

// CHECK-LABEL: define{{.*}} @_Z5daddrP1SMS_i(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a6.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret ptr
int *daddr(S *a6, int S::*pm) { return &((*a6).*pm); }

// CHECK-LABEL: define{{.*}} @_Z11member_addrP1TM1Si(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %a7.addr
// INDIR-NEXT: icmp eq ptr [[P]], null
// ACCESS-NOT: call void @llvm.trap()
// CHECK: ret ptr
int *member_addr(T *a7, int S::*pm) { return &(a7->s.*pm); }
