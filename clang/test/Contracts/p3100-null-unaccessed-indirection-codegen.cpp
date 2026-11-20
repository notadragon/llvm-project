// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ACC

// P3100: under quick_enforce, each unaccessed indirection through a pointer
// that may be null gets one guard under the .indirection leaf (CLANG-753);
// `this`, the address of an object, an array and a reference are not
// checked, and .access alone guards none.  A constexpr call in an `if`
// condition that would fold to a constant is emitted when the pointer is
// null.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-unaccessed-indirection-codegen.C.)

struct S {
  static int k;
  static int sf();
  static constexpr int csf() { return 3; }
  int m;
  int g();
};

// CHECK-LABEL: define{{.*}} @_ZN1S1gEv(
// CHECK-NOT: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_ZN1S1gEv(
// ACC-NOT: llvm.trap
// ACC: ret i32
int S::g() {
  (void)*this;
  S *q = &*this;
  return this->sf() + (*this).sf() + this->k + (q == this);
}

// CHECK-LABEL: define{{.*}} @_Z4addrPi(
// CHECK: icmp eq ptr
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
// ACC-LABEL: define{{.*}} @_Z4addrPi(
// ACC-NOT: llvm.trap
// ACC: ret ptr
int *addr(int *p) { return &*p; }

// CHECK-LABEL: define{{.*}} @_Z4discPi(
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret void
// ACC-LABEL: define{{.*}} @_Z4discPi(
// ACC-NOT: llvm.trap
// ACC: ret void
void disc(int *p) { *p; }

// CHECK-LABEL: define{{.*}} @_Z3sfcP1S(
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_Z3sfcP1S(
// ACC-NOT: llvm.trap
// ACC: ret i32
int sfc(S *sp) { return sp->sf(); }

// CHECK-LABEL: define{{.*}} @_Z2skP1S(
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret i32
// ACC-LABEL: define{{.*}} @_Z2skP1S(
// ACC-NOT: llvm.trap
// ACC: ret i32
int sk(S *sp) { return sp->k; }

// CHECK-LABEL: define{{.*}} @_Z5addr0Pi(
// CHECK: llvm.trap
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
int *addr0(int *p) { return &p[0]; }

// Controls.
int x;
int arr[2];
// CHECK-LABEL: define{{.*}} @_Z8addr_objv(
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
int *addr_obj() { return &*&x; }
// CHECK-LABEL: define{{.*}} @_Z8addr_arrv(
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
int *addr_arr() { return &*arr; }
// CHECK-LABEL: define{{.*}} @_Z7sfc_refR1S(
// CHECK-NOT: llvm.trap
// CHECK: ret i32
int sfc_ref(S &r) { return r.sf(); }
// CHECK-LABEL: define{{.*}} @_Z8disc_refRi(
// CHECK-NOT: llvm.trap
// CHECK: ret void
void disc_ref(int &r) { r; }

// The call is emitted, not folded to 3.
constexpr int csfc(S *sp) { return sp->csf(); }
// CHECK-LABEL: define{{.*}} @_Z6foldedv(
// CHECK: call{{.*}} @_Z4csfcP1S(ptr noundef null)
// CHECK: ret i32
int folded() {
  if (csfc(nullptr) == 3)
    return 1;
  return 0;
}
