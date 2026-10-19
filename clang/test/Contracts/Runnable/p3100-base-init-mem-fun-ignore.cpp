// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.mem.init:ignore \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a member function called in a base-class mem-initializer
// (ub:class.base.init.mem.fun.nonpolymorphic.mem.init) configured to "ignore":
// no handler call, and execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-base-init-mem-fun-ignore.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
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
  Derived d;
  std::printf("RESULT=%d\n", d.v == 7);
  return 0;
}
// CHECK-NOT: VIOL
// CHECK: RESULT=1
