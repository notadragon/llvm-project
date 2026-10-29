// P3100 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=vptr a member function call through a glvalue whose
// type is not similar to the object's (ub:expr.ref.member.not.similar,
// polymorphic) resolves to noexcept_enforce: the handler runs (kind 7,
// semantic 7) and the program terminates.
// (GCC mirror: g++.dg/ubsan/p3100-vptr-member-not-similar-route-enforce.C)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr -fno-sanitize-recover=vptr %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

struct A {
  virtual ~A() {}
  int i = 1;
};
struct B {
  virtual ~B() {}
  int j = 2;
  int f() { return 30; }
};

__attribute__((noinline)) int call(A &a) {
  return reinterpret_cast<B &>(a).f();
}

int main() {
  A a;
  volatile int sink = call(a);
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
