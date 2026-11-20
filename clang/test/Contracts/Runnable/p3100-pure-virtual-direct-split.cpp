// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: configuring only the .direct leaf of ub:class.abstract.pure.virtual
// does not check a pure virtual function called without qualification in its
// class's constructor: Clang dispatches the call (the .indirect leaf, see
// p3100-pure-virtual-direct-edges.cpp), so the vtable slot stays the stock
// __cxa_pure_virtual, which aborts without calling the handler; the
// definition does not run.  GCC resolves the call statically and checks it
// as .direct.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-pure-virtual-direct-enforce.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
struct B {
  B() { f(); }
  virtual void f() = 0;
  virtual ~B() {}
};
void B::f() {
  std::printf("DEFINITION\n");
  std::fflush(stdout);
}
struct D : B {
  void f() override {}
};
int main() {
  { D d; }
  std::printf("RESULT=1\n");
  return 0;
}
// CHECK-NOT: VIOL
// CHECK-NOT: DEFINITION
// CHECK: Pure virtual function called!
// CHECK-NOT: RESULT
