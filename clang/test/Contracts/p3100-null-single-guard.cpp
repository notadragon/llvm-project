// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-null.json -emit-llvm -o - %s | FileCheck %s

// Each null dereference is guarded once, so an observing semantic reports it
// once: a load through a member access, an accessed subscript or `->*' is not
// checked again on the member's or element's address (which coincides with
// the pointer for the first member or index 0), and a member function does
// not check its own `this' (the call site checks the object expression).

struct S {
  int a;
  int g() { return 1; }
};

int first(S *p) { return p->a; }
// CHECK-LABEL: define {{.*}}@_Z5firstP1S(
// CHECK: {{^}}nulldref.viol:
// CHECK-NOT: {{^}}nulldref.viol{{[0-9]+}}:
// CHECK: ret i32

int sub0(int *p) { return p[0]; }
// CHECK-LABEL: define {{.*}}@_Z4sub0Pi(
// CHECK: {{^}}nulldref.viol:
// CHECK-NOT: {{^}}nulldref.viol{{[0-9]+}}:
// CHECK: ret i32

int ptm(S *p, int S::*pm) { return p->*pm; }
// CHECK-LABEL: define {{.*}}@_Z3ptmP1SMS_i(
// CHECK: {{^}}nulldref.viol:
// CHECK-NOT: {{^}}nulldref.viol{{[0-9]+}}:
// CHECK: ret i32

int call(S *p) { return p->g(); }
// CHECK-LABEL: define {{.*}}@_Z4callP1S(
// CHECK: {{^}}nulldref.viol:
// CHECK-NOT: {{^}}nulldref.viol{{[0-9]+}}:
// CHECK: ret i32

// CHECK-LABEL: define {{.*}}@_ZN1S1gEv(
// CHECK-NOT: nulldref.viol
// CHECK: ret i32
