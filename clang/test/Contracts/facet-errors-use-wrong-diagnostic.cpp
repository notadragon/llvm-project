// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s

// The two P3400 label errors (no allowed semantics; a compute_semantic result
// outside them) have diagnostics of their own, naming the label type.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/facet-diagnostic-text.C

#include <contracts>

using std::contracts::evaluation_semantic;

struct bad_t {
  using assertion_control_object = bad_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics{
      evaluation_semantic::observe};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::enforce;
  }
};
constexpr bad_t bad{};

// expected-error-re@+1 {{{{^}}{{'?}}compute_semantic{{'?}} result is not in the allowed evaluation semantics}}
void f(int x) pre<bad>(x > 0) {}

// REQUIRES: contracts-libcxx
