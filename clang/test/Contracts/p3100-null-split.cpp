// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce \
// RUN:   | FileCheck %s --check-prefixes=CHECK,ACCESS
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce \
// RUN:   | FileCheck %s --check-prefixes=CHECK,INDIR
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:quick_enforce \
// RUN:   | FileCheck %s --check-prefixes=CHECK,PARENT

// P3100: the null-dereference check reports the .access leaf of
// ub:expr.unary.dereference.nullptr where the indirection is followed by a
// load or store (*p, p->m read or written, p->arr[i] read, p->*pm read), and
// the .indirection leaf where it is not (a reference bound to *p, a member
// call p->f(), the address of a member &p->n or &p->arr[i]).  Configuring one
// leaf guards only its shapes; the parent group still selects both.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-null-split-access.C,
// p3100-null-split-indirection.C and p3100-null-split-member-access.C,
// -indirection.C and -parent.C; the pointer-to-member shapes are
// p3100-null-split-mptr.cpp here, p3100-null-split-mptr-access.C and
// -indirection.C there.  Runnable/p3100-null-split-mptr-offset.cpp pins the
// .access check on a null p with a member at a nonzero offset.  The
// subscript shapes are p3100-null-split-subscript-access.C, -indirection.C
// and -parent.C there, which check p + i rather than p -- CLANG-745, gnu_gcc
// GCC-794.)

struct S { int m; int n; int arr[4]; void f(); };

// CHECK-LABEL: define{{.*}} @_Z10shape_loadPi(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int shape_load(int *p) { return *p; }

// CHECK-LABEL: define{{.*}} @_Z9shape_refPi(
// ACCESS-NOT: call void @llvm.trap()
// INDIR: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int &shape_ref(int *p) { int &r = *p; return r; }

// CHECK-LABEL: define{{.*}} @_Z10shape_callP1S(
// ACCESS-NOT: call void @llvm.trap()
// INDIR: call void @llvm.trap()
// PARENT: call void @llvm.trap()
void shape_call(S *p) { p->f(); }

// CHECK-LABEL: define{{.*}} @_Z9shape_memP1S(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int shape_mem(S *p) { return p->m; }

// CHECK-LABEL: define{{.*}} @_Z10shape_addrP1S(
// ACCESS-NOT: call void @llvm.trap()
// INDIR: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int *shape_addr(S *p) { return &p->n; }

// CHECK-LABEL: define{{.*}} @_Z11shape_storeP1S(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
void shape_store(S *p) { p->n = 1; }

// CHECK-LABEL: define{{.*}} @_Z16shape_arr_accessP1Si(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int shape_arr_access(S *p, int i) { return p->arr[i]; }

// CHECK-LABEL: define{{.*}} @_Z14shape_arr_addrP1Si(
// ACCESS-NOT: call void @llvm.trap()
// INDIR: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int *shape_arr_addr(S *p, int i) { return &p->arr[i]; }

// CHECK-LABEL: define{{.*}} @_Z10shape_mptrP1SMS_i(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int shape_mptr(S *p, int S::*pm) { return p->*pm; }

// CHECK-LABEL: define{{.*}} @_Z11shape_chainP1T(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
struct T { S s; };
int shape_chain(T *p) { return p->s.m; }

// CHECK-LABEL: define{{.*}} @_Z16shape_chain_addrP1T(
// ACCESS-NOT: call void @llvm.trap()
// INDIR: call void @llvm.trap()
// PARENT: call void @llvm.trap()
int *shape_chain_addr(T *p) { return &p->s.m; }

// The object expression of a member access that is a subscript, p[i].n:
// `p` itself is checked, .access where the member is read, .indirection
// where only its address is formed (CLANG-744); and an element of a
// two-dimensional member array likewise (the inner subscript's member
// access takes the outer one's leaf).
// CHECK-LABEL: define{{.*}} @_Z12shape_sub_rdP1Si(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %p.addr
// ACCESS: icmp eq ptr [[P]], null
// INDIR-NOT: call void @llvm.trap()
// PARENT: icmp eq ptr [[P]], null
// CHECK: ret i32
int shape_sub_rd(S *p, int i) { return p[i].n; }

// CHECK-LABEL: define{{.*}} @_Z14shape_sub_addrP1Si(
// CHECK: [[P:%[0-9]+]] = load ptr, ptr %p.addr
// ACCESS-NOT: call void @llvm.trap()
// INDIR: icmp eq ptr [[P]], null
// PARENT: icmp eq ptr [[P]], null
// CHECK: ret ptr
int *shape_sub_addr(S *p, int i) { return &p[i].n; }

struct U { int z; int a2[2][2]; };

// CHECK-LABEL: define{{.*}} @_Z13shape_arr2_rdP1Uii(
// ACCESS: call void @llvm.trap()
// INDIR-NOT: call void @llvm.trap()
// PARENT: call void @llvm.trap()
// CHECK: ret i32
int shape_arr2_rd(U *p, int i, int j) { return p->a2[i][j]; }

// CHECK-LABEL: define{{.*}} @_Z15shape_arr2_addrP1Uii(
// ACCESS-NOT: call void @llvm.trap()
// INDIR: call void @llvm.trap()
// PARENT: call void @llvm.trap()
// CHECK: ret ptr
int *shape_arr2_addr(U *p, int i, int j) { return &p->a2[i][j]; }
