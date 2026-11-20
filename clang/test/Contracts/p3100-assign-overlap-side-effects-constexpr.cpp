// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:enforce
// expected-no-diagnostics

// P3100: with the overlap check (ub:expr.assign.overlap) configured, an
// assignment whose operands have side effects is still a constant
// expression, evaluated as without the check: the check is made by code
// generation.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-side-effects-constexpr.C)

constexpr int *at(int *p, int i, int &calls) {
  ++calls;
  return p + i;
}
constexpr int f() {
  int a[3]{1, 2, 3};
  int k = 0;
  a[k++] = a[k + 1];
  int calls = 0;
  *at(a, 2, calls) = *at(a, 1, calls);
  return a[0] * 1000 + a[2] * 100 + k * 10 + calls;
}
static_assert(f() == 2000 + 200 + 10 + 2);
