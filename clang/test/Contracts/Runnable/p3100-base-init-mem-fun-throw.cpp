// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.mem.init:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at a member function called in a base-
// class mem-initializer (ub:class.base.init.mem.fun.nonpolymorphic.mem.init)
// under "enforce": the exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-base-init-mem-fun-throw.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
}
struct Base {
  int v;
  Base(int y) : v(y) {}
};
struct Derived : Base {
  int j;
  int f() const { return 7; } // reads no member
  Derived() : Base(f()), j(5) {}
};
int main() {
  try {
    Derived d;
    std::printf("RESULT=%d\n", d.v == 7);
  } catch (Boom &) {
    std::printf("CAUGHT\n");
    std::fflush(stdout);
  }
  std::printf("SURVIVED\n");
  return 0;
}
// CHECK: VIOL sem=3
// CHECK-NEXT: CAUGHT
// CHECK-NEXT: SURVIVED
