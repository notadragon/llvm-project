// RUN: %clangxx -std=c++17 %s -fsanitize=vptr %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-752 in this fork's bug-reports/).  Stock Clang 18 to trunk;
// GCC reports every form.
//
// -fsanitize=vptr (and so the routed ub:expr.ref.member.not.similar) does not
// check a data member read through a reference variable or parameter bound
// to a glvalue of the wrong polymorphic type: EmitCheckedLValue skips every
// type check of a DeclRefExpr object expression, the vptr check with the
// null check.  The cast used directly, a pointer and a member function call
// through the reference are reported (the controls).  When every form is
// reported, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/ubsan/vptr-reinterpret-reference.C, a passing test
// there: stock GCC -fsanitize=vptr reports all six forms -- the direct,
// reference-variable, pointer and member-call forms, `B &&`, `B &` param.)

#include <cstdio>

struct A {
  virtual ~A() {}
  int i = 1, j = 2;
};
struct B {
  virtual ~B() {}
  int i = 1, j = 2;
  __attribute__((noinline)) int f() { return 3; }
};

__attribute__((noinline)) int direct(A &a) {
  return reinterpret_cast<B &>(a).j;
}
__attribute__((noinline)) int viaptr(A &a) {
  B *b = reinterpret_cast<B *>(&a);
  return b->j;
}
__attribute__((noinline)) int viacall(A &a) {
  B &b = reinterpret_cast<B &>(a);
  return b.f();
}
__attribute__((noinline)) int viaref(A &a) {
  B &b = reinterpret_cast<B &>(a);
  return b.j;
}
__attribute__((noinline)) int viarvref(A &a) {
  B &&b = reinterpret_cast<B &&>(a);
  return b.j;
}
__attribute__((noinline)) int param(B &b) { return b.j; }

int main() {
  A a;
  std::fprintf(stderr, "direct\n");
  direct(a);
  std::fprintf(stderr, "viaptr\n");
  viaptr(a);
  std::fprintf(stderr, "viacall\n");
  viacall(a);
  std::fprintf(stderr, "viaref\n");
  viaref(a);
  std::fprintf(stderr, "viarvref\n");
  viarvref(a);
  std::fprintf(stderr, "param\n");
  param(reinterpret_cast<B &>(a));
  std::fprintf(stderr, "end\n");
  return 0;
}

// CHECK-LABEL: direct
// CHECK: runtime error: member access within address {{.*}} which does not point to an object of type 'B'
// CHECK-LABEL: viaptr
// CHECK: runtime error: member access within address {{.*}} which does not point to an object of type 'B'
// CHECK-LABEL: viacall
// CHECK: runtime error: member call on address {{.*}} which does not point to an object of type 'B'
// CHECK-LABEL: viaref
// CHECK: runtime error: member access within address {{.*}} which does not point to an object of type 'B'
// CHECK-LABEL: viarvref
// CHECK: runtime error: member access within address {{.*}} which does not point to an object of type 'B'
// CHECK-LABEL: param
// CHECK: runtime error: member access within address {{.*}} which does not point to an object of type 'B'
// CHECK-LABEL: end
