// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3100 %libcxx_flags \
// RUN:   -fsyntax-only -Xclang -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:enforce
// REQUIRES: contracts-libcxx
// expected-no-diagnostics

// P3100: with the placement-new alignment check configured, constant
// evaluation is unchanged: a placement new that stores nothing stays a
// constant expression.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-align-constexpr.C)

#include <new>
struct S {
  int a, b;
};
constexpr int f() {
  S s{1, 2};
  S *p = new (&s) S;
  p->a = 3;
  p->b = 4;
  return p->a + p->b;
}
static_assert(f() == 7);
