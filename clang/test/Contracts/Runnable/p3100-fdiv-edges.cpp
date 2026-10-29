// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: floating-point division by zero (ub:expr.mul.div.by.zero.fp.div)
// under "observe": a -0.0 divisor compares equal to zero and is caught; a NaN
// divisor and a nonzero one are not; float, long double, 0.0 / 0.0 and the
// compound assignment /= are caught.  Each caught division yields 0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fdiv-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
__attribute__((noinline)) double div_it(double a, double b) { return a / b; }
volatile double zero = 0.0;
int main() {
  double r1 = div_it(1.0, 0.0);
  double r2 = div_it(1.0, -0.0);
  double r3 = div_it(1.0, __builtin_nan(""));
  double r4 = div_it(6.0, 2.0);
  float f = 2.0f;
  f /= (float)zero;
  long double ld = 1.0L / (long double)zero;
  double r5 = 0.0 / zero;
  std::printf("RESULT %g %g %d %g %g %g %g viols=%d\n", r1, r2,
              (int)__builtin_isnan(r3), r4, (double)f, (double)ld, r5, viols);
  return 0;
}
// CHECK: RESULT 0 0 1 3 0 0 0 viols=5
