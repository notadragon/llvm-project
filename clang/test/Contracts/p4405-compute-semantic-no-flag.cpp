// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only -Xclang -verify %s

// P4405: without -fcontracts-p4405, prove and diagnose are not in any
// contract's allowed set, so a compute_semantic facet returning one is an
// error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-compute-semantic-no-flag.C

#include <contracts>

using std::contracts::evaluation_semantic;

template <evaluation_semantic R>
struct to {
  using assertion_control_object = to;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return R;
  }
};

constexpr to<evaluation_semantic::prove> to_prove{};
constexpr to<evaluation_semantic::diagnose> to_diagnose{};

int a(int x) pre<to_prove>(x > 0) { return x; }    // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}
int b(int x) pre<to_diagnose>(x > 0) { return x; } // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}

// REQUIRES: contracts-libcxx
