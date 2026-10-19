// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-737 in this fork's bug-reports/).  Stock Clang and GCC
// alike (gnu_gcc GCC-749).
//
// `sp->sf()` and `sp->k` with `sp` null evaluate `*sp` ([expr.ref]: `E1->E2`
// is `(*E1).E2`, and the object expression is evaluated even when the member
// is static), an indirection through a null pointer, which is undefined
// ([expr.unary.op]/1, CWG2823), so neither f() nor g() is a core constant
// expression; Clang accepts both.  The explicit form `(*sp).sf()`, a
// discarded `*p` and `&*p` are rejected
// (../constexpr-null-indirection-no-read.cpp).  Both forms take the same path
// (LValueExprEvaluator::VisitMemberExpr evaluates the base pointer as an
// ignored value), so one file.  When both are rejected, this XFAIL becomes an
// XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-null-indirection-no-read.C)

struct S {
  static constexpr int sf() { return 1; }
  static constexpr int k = 1;
};
constexpr bool f() {
  S *sp = nullptr;
  sp->sf();
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}}

constexpr bool g() {
  S *sp = nullptr;
  return sp->k == 1;
}
static_assert(g()); // expected-error {{not an integral constant expression}}
