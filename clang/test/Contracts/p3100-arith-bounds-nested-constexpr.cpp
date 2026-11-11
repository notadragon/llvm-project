// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:enforce \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:enforce

// P3100: with the known-bound pointer-arithmetic checks configured, constant
// evaluation is unchanged: nested arithmetic, an indirection through it and a
// subscript of it within the array are still constant expressions, and nested
// arithmetic past the bound is rejected by the evaluator as it always was.
// (GCC's evaluator accepts arithmetic past N, so there the check reports it as
// undefined.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-nested-constexpr.C)

constexpr int f(int i, int j) {
  int a[4]{1, 2, 3, 4};
  int *p = (a + i) + j;
  int *q = &(a + 1)[j];
  return *(a + i + j - 1) + (a + 1)[i] + (p - q);
}
static_assert(f(1, 2) == 3 + 3 + 0);
static_assert(f(2, 2) == 4 + 4 + 1);

constexpr bool g(int i) { int a[3]{}; int *p = (a + 1) + i; return p != nullptr; } // expected-note {{cannot refer to element 4 of array of 3 elements in a constant expression}}
static_assert(g(2));
constexpr bool b = g(3); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'g(3)'}}
