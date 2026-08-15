// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type \
// RUN:   -fcontract-configuration-file=%S/p3100-null-ref-and-call.json \
// RUN:   -S -emit-llvm -o - %libcxx_flags | FileCheck %s

// P3100: routing a null-dereference check must cover every access shape,
// not just a plain load or store.
//
// The P3100 guard in EmitTypeCheck was gated on TCK_Load || TCK_Store, so
// binding a reference to *p and calling a non-static member function
// through a possibly-null this never reached it -- the same undefined
// behaviour, silently unenforced by syntax.  The reference-binding call
// site was additionally gated on sanitizePerformTypeCheck(), so widening
// the guard alone was not enough to reach it.
//
// Clang checks 'this' once per function in the callee rather than at the
// call, so S::f carries its own guard; that is a placement difference from
// GCC, not a missing check.
//
// GCC mirror: g++.dg/contracts/cpp26/p3100-null-ref-and-call.C
// (gnu_gcc b20fde9aff5).

struct S {
  int m;
  void f();
};

void S::f() {}

// Reference binding: regressed shape.
// CHECK-LABEL: define{{.*}} @_Z9shape_refPi
// CHECK: llvm.trap
int &shape_ref(int *p) {
  int &r = *p;
  return r;
}

// Plain load: already worked.
// CHECK-LABEL: define{{.*}} @_Z10shape_loadPi
// CHECK: llvm.trap
int shape_load(int *p) { return *p; }

// Taking the address of a dereference accesses nothing and must stay
// uninstrumented -- see p3100-null-deref-addr-of.cpp.
// CHECK-LABEL: define{{.*}} @_Z7addr_ofPi
// CHECK-NOT: llvm.trap
// CHECK: ret ptr
int *addr_of(int *p) { return &*p; }
