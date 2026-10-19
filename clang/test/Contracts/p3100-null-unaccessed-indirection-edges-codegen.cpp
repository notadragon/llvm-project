// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ACC

// P3100: under quick_enforce, each of the unaccessed indirections of
// Runnable/p3100-null-unaccessed-indirection-edges.cpp through a pointer
// that may be null gets one guard under the .indirection leaf and none under
// .access: the address of a conditional or comma expression yielding `*p` or
// of `*p` cast to a reference, the same discarded (also a conversion to a
// base at a nonzero offset), `__builtin_addressof(*p)`, an enumerator, a
// pseudo-destructor and a trivial destructor named through `p` (CLANG-759).
// A discarded conversion of `*p` to a virtual base reads through p, so it is
// the .access leaf's; the address of an object, a reference and `this` are
// not checked.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-unaccessed-indirection-edges-codegen.C,
// which counts the same nine guards.)

struct S {
  int m;
  enum { E = 5 };
  static int k;
  int g();
};
struct A { int a; };
struct D : A, S {};
struct V : virtual S {};
using I = int;

// CHECK-LABEL: define{{.*}} @_Z9addr_condPiS_b(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z9addr_condPiS_b(
// ACC-NOT: llvm.trap
// ACC: ret ptr
int *addr_cond(int *p, int *q, bool b) { return &(b ? *p : *q); }

// CHECK-LABEL: define{{.*}} @_Z10addr_commaPi(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z10addr_commaPi(
// ACC-NOT: llvm.trap
// ACC: ret ptr
int *addr_comma(int *p) { return &(0, *p); }

// CHECK-LABEL: define{{.*}} @_Z9addr_castP1S(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z9addr_castP1S(
// ACC-NOT: llvm.trap
// ACC: ret ptr
S *addr_cast(S *sp) { return &static_cast<S &>(*sp); }

// CHECK-LABEL: define{{.*}} @_Z9disc_castP1S(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z9disc_castP1S(
// ACC-NOT: llvm.trap
// ACC: ret void
void disc_cast(S *sp) { static_cast<S &>(*sp); }

// CHECK-LABEL: define{{.*}} @_Z13disc_base_offP1D(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z13disc_base_offP1D(
// ACC-NOT: llvm.trap
// ACC: ret void
void disc_base_off(D *dp) { static_cast<S &>(*dp); }

// CHECK-LABEL: define{{.*}} @_Z5baddrPi(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z5baddrPi(
// ACC-NOT: llvm.trap
// ACC: ret ptr
int *baddr(int *p) { return __builtin_addressof(*p); }

// CHECK-LABEL: define{{.*}} @_Z2seP1S(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_Z2seP1S(
// ACC-NOT: llvm.trap
// ACC: ret i32
int se(S *sp) { return sp->E; }

// CHECK-LABEL: define{{.*}} @_Z5pdtorPi(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z5pdtorPi(
// ACC-NOT: llvm.trap
// ACC: ret void
void pdtor(int *p) { p->~I(); }

// CHECK-LABEL: define{{.*}} @_Z5sdtorP1S(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z5sdtorP1S(
// ACC-NOT: llvm.trap
// ACC: ret void
void sdtor(S *sp) { sp->~S(); }

// Not checked, but for the virtual base's offset read under .access.
// CHECK-LABEL: define{{.*}} @_Z10disc_vbaseP1V(
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z10disc_vbaseP1V(
// ACC: llvm.trap
// ACC-NOT: llvm.trap
// ACC: ret void
void disc_vbase(V *vp) { static_cast<S &>(*vp); }
int x, y;

// CHECK-LABEL: define{{.*}} @_Z13addr_obj_condb(
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z13addr_obj_condb(
// ACC-NOT: llvm.trap
// ACC: ret ptr
int *addr_obj_cond(bool b) { return &(b ? x : y); }

// CHECK-LABEL: define{{.*}} @_Z13disc_ref_castRi(
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z13disc_ref_castRi(
// ACC-NOT: llvm.trap
// ACC: ret void
void disc_ref_cast(int &r) { static_cast<int &>(r); }

// CHECK-LABEL: define{{.*}} @_ZN1S1gEv(
// CHECK-NOT: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_ZN1S1gEv(
// ACC-NOT: llvm.trap
// ACC: ret i32
int S::g() { static_cast<S &>(*this); return (*this).E + this->k; }
