// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// P3400: as p3400-compute-semantic-no-header.cpp, with <contracts> included
// after the label's use.  The enumeration std::contracts::evaluation_semantic
// is declared there and must not conflict with anything the label's use left
// behind.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-header-after.C
// (GCC-676).

struct L {
  using assertion_control_object = L;
  constexpr int compute_semantic(int s) const { return s; }
};

constexpr L l;

void f(int x) pre<l> (x > 0) {}

#include <contracts>

void g(int x) pre (x > 0) {}
