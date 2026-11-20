// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:enforce

// P3100: with the floating-point division-by-zero check configured, constant
// evaluation is unchanged: a division by a nonzero value is a constant
// expression, and a division by zero is still rejected by the evaluator.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fdiv-constexpr.C)

constexpr double dz(double z) { return 1.0 / z; } // expected-note {{division by zero}}
static_assert(dz(2.0) == 0.5);
constexpr double d = dz(0.0); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'dz(0.000000e+00)'}}
