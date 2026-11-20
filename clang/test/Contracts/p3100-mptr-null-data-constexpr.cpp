// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:enforce

// P3100: with the null pointer-to-data-member check configured, constant
// evaluation is unchanged: applying a non-null pointer stays a constant
// expression, and applying a null one is still rejected by the evaluator.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-data-constexpr.C)

struct S {
  int a;
  int m;
};
constexpr int get(int S::*p) {
  S s{1, 2};
  return s.*p; // expected-note {{subexpression not valid in a constant expression}}
}
constexpr bool same(int S::*p) {
  S s{};
  return &(s.*p) == &s.m;
}
static_assert(get(&S::a) == 1);
static_assert(get(&S::m) == 2);
static_assert(same(&S::m));
constexpr int bad = get(nullptr); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'get(0)'}}
