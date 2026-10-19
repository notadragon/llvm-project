// P3100 (UBSan runtime routing): a class member access or member function
// call whose object expression is not similar to the type of the object
// (ub:expr.ref.member.not.similar), for polymorphic classes, is caught by
// -fsanitize=vptr ("member access within" / "member call on") and routed to
// the contract-violation handler.  vptr is recoverable, so with
// -fcontracts-p4298 it resolves to noexcept_observe: the handler runs (kind 7,
// semantic 6) once per access and the program continues.  Accesses through
// the object's own type, through a base of it, or to an object of a derived
// type do not report.
// (GCC mirror: g++.dg/ubsan/p3100-vptr-member-not-similar-route-observe.C)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

static int viols;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++viols;
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

// Two unrelated polymorphic classes with the same layout.
struct A {
  virtual ~A() {}
  virtual int w() { return 10; }
  int i = 1;
};
struct B {
  virtual ~B() {}
  virtual int v() { return 20; }
  int j = 2;
  int f() { return 30; }
};
struct D : A {
  int d = 3;
};

__attribute__((noinline)) int dot(A &a) { return reinterpret_cast<B &>(a).j; }
__attribute__((noinline)) int arrow(A *a) {
  return reinterpret_cast<B *>(a)->j;
}
__attribute__((noinline)) int call(A &a) {
  return reinterpret_cast<B &>(a).f();
}
__attribute__((noinline)) int vcall(A &a) {
  return reinterpret_cast<B &>(a).v();
}
__attribute__((noinline)) int same(A &a) { return reinterpret_cast<A &>(a).i; }
__attribute__((noinline)) int base(D &d) { return static_cast<A &>(d).i; }
__attribute__((noinline)) int derived(A &a) { return a.i + a.w(); }

int main() {
  A a;
  D d;
  same(a);
  base(d);
  derived(d);
  std::printf("clean=%d\n", viols);
  std::fflush(stdout);
  dot(a);
  arrow(&a);
  call(a);
  vcall(a);
  std::printf("survived=%d\n", viols);
  std::fflush(stdout);
  return 0;
}

// Nothing from UBSan itself: the handler lines are the only output.
// CHECK-NOT: runtime error
// CHECK: clean=0
// CHECK-NEXT: handler kind=7 semantic=6
// CHECK-NEXT: handler kind=7 semantic=6
// CHECK-NEXT: handler kind=7 semantic=6
// CHECK-NEXT: handler kind=7 semantic=6
// CHECK-NEXT: survived=4
