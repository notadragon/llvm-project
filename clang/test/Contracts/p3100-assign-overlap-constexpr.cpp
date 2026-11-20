// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:enforce
// expected-no-diagnostics

// P3100: with the overlapping assignment check configured
// (ub:expr.assign.overlap), constant evaluation is unchanged: the check is
// made by code generation, so the assignment from an overlapping union member
// stays a constant expression, as it is without the check (the stock miss
// CLANG-716).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-constexpr.C)

union U {
  int i;
  short s;
};
constexpr int un() {
  U u;
  u.s = 1;
  u.i = u.s;
  return u.i;
}
static_assert(un() == 1);
constexpr int ok(int *p, int *q) {
  *p = *q;
  return *p;
}
constexpr int run() {
  int a = 1, b = 2;
  return ok(&a, &b) + ok(&a, &a);
}
static_assert(run() == 4);
