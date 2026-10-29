// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:quick_enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: floating-point division by zero (ub:expr.mul.div.by.zero.fp.div)
// configured to "quick_enforce": terminates without calling the handler.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fdiv-quick.C)

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
// CHECK-NOT: VIOL
// CHECK-NOT: RESULT
