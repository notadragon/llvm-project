// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual:noexcept_observe \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: configuring the parent group ub:class.abstract.pure.virtual checks a
// pure virtual function called without qualification in its class's
// destructor: Clang dispatches it to the terminus (the .indirect leaf), which
// under noexcept_observe reports (kind=7, sem=6) and then aborts; the
// definition does not run.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-pure-virtual-direct-group.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
struct B {
  virtual void f() = 0;
  virtual ~B() { f(); }
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
// CHECK: VIOL kind=7 sem=6 comment=[pure virtual function called]
// CHECK-NOT: DEFINITION
// CHECK-NOT: RESULT
