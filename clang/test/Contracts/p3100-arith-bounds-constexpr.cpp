// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce

// P3100: with the known-bound pointer-arithmetic check configured, constant
// evaluation is unchanged: arithmetic within [0, N] is a constant expression,
// and arithmetic past N is rejected by the evaluator as it always was.  (GCC's
// evaluator accepts arithmetic past N, so there the check reports it as
// undefined.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-constexpr.C)

constexpr bool f(int i) { int a[3]{}; int *p = a + i; return p != nullptr; } // expected-note {{cannot refer to element 4 of array of 3 elements in a constant expression}}
static_assert(f(3));
constexpr bool b = f(4); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'f(4)'}}
