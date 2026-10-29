// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// In the P3400 wording an assertion-control-expression is a
// postfix-expression alternative ([expr.post.general]), so member access may
// follow it directly.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-control-postfix.C
// in the gnu_gcc fork (expected pass).

struct L { using assertion_control_object = L; int v; };
constexpr L make() { return L{3}; }

constexpr int q = contract_control(make()).v; // OK, a postfix-expression
static_assert(q == 3);
