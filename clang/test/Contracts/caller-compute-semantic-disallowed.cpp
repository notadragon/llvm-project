// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only -Xclang -verify %s

// A label's compute_semantic returns ignore, which its allowed_semantics
// excludes: diagnosed at the declaration.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/caller-compute-semantic-disallowed.C

#include <contracts>
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct L {
  using assertion_control_object = L;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set{evaluation_semantic::observe,
                              evaluation_semantic::enforce};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::ignore;
  }
};
constexpr L l{};

void f(int x) pre<l>(x > 0); // expected-error {{not in the allowed evaluation semantics}}
void g() { f(1); }

// REQUIRES: contracts-libcxx
