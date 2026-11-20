// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only -Xclang -verify %s

// A label's compute_semantic returns ignore, which its allowed_semantics
// excludes: diagnosed at the declaration.  gnu_gcc's test checks the
// caller-side resolution, where D4405 ("Adjusting to the Allowed
// Semantics") lets a caller-side evaluation always be ignore -- the
// callee-side one is made with an allowed semantic -- so ignore is accepted
// there.  Clang has no caller-side checking and applies no compute_semantic
// to a caller-side evaluation, so this checks the callee side, whose
// allowed set is R, {observe, enforce} here, without ignore.  A result
// outside every set (assume, not allowed without -fcontracts-allow-assume)
// is diagnosed either way.
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

struct M {
  using assertion_control_object = M;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set{evaluation_semantic::observe,
                              evaluation_semantic::enforce};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::assume;
  }
};
constexpr M m{};

void f(int x) pre<l>(x > 0); // expected-error {{not in the allowed evaluation semantics}}
void g() { f(1); }

void h(int x) pre<m>(x > 0); // expected-error {{not in the allowed evaluation semantics}}
void k() { h(1); }

// REQUIRES: contracts-libcxx
