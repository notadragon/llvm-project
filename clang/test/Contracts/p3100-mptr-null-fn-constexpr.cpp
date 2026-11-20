// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:enforce

// P3100: with the null pointer-to-member-function check configured, constant
// evaluation is unchanged: a call through a non-null pointer is a constant
// expression, and a call through a null one is still rejected by the
// evaluator, not as a call to the violation entry point.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-fn-constexpr.C)

struct S {
  constexpr int f() const { return 1; }
  constexpr virtual int g() const { return 2; }
};
constexpr int call(int (S::*p)() const) {
  S s;
  return (s.*p)(); // expected-note {{subexpression not valid in a constant expression}}
}
static_assert(call(&S::f) == 1);
static_assert(call(&S::g) == 2);
constexpr int bad = call(nullptr); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'call(0)'}}
