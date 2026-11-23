// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at floating-point division by zero
// (ub:expr.mul.div.by.zero.fp.div) under "enforce": the site is in a function
// that may throw, so the exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fdiv-throw.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
}
__attribute__((noinline)) double div_it(double a, double b) { return a / b; }
int main() {
  try {
    std::printf("RESULT=%g\n", div_it(1.0, 0.0));
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
