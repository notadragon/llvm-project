// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3400: a label with a compute_semantic member, in a translation unit that
// never includes <contracts>.  Without std::contracts::evaluation_semantic
// there is no facet to call, so the member is not one and the label's use
// is accepted.  p3400-compute-semantic-header-after.cpp includes <contracts>
// after the label's use.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-no-header.C
// (GCC-676, where probing the facet crashed).

struct L {
  using assertion_control_object = L;
  constexpr int compute_semantic(int s) const { return s; }
};

constexpr L l;

void f(int x) pre<l> (x > 0) {}
