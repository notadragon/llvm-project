// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcxx-exceptions -fsyntax-only \
// RUN:   -Wno-invalid-noreturn -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:enforce
// expected-no-diagnostics

// P3100: with the [[noreturn]] check configured, constant evaluation is
// unchanged: a constexpr function that calls a [[noreturn]] constexpr
// function only on a path it does not take stays a constant expression.
// (Clang has no constexpr exceptions, so GCC's throwing path cannot be
// mirrored; a returning call is accepted in constant evaluation, stock and
// here alike: CLANG-711.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noreturn-constexpr.C)

[[noreturn]] constexpr void nr(int x) {
  if (x > 0)
    return;
  throw x;
}
constexpr int f(int x) {
  if (x < 0)
    nr(x);
  return 1;
}
static_assert(f(0) == 1);
