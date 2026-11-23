// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: floating-point division by zero (ub:expr.mul.div.by.zero.fp.div)
// configured to "observe": the handler runs (kind=7, sem=2) and execution
// continues with the defined substitute.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fdiv-observe.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
__attribute__((noinline)) double div_it(double a, double b) { return a / b; }
int main() {
  std::printf("RESULT=%g\n", div_it(1.0, 0.0));
  return 0;
}
// CHECK: VIOL kind=7 sem=2 comment=[floating-point division by zero]
// CHECK-NEXT: RESULT=0
