// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only

// P3400 x P4298: a label's allowed_semantics must be able to permit the P4298
// noexcept variants.  A label allowing only noexcept_enforce (and selecting it
// via compute_semantic) must be accepted under -fcontracts-p4298 -- the allowed
// set is non-empty, so no "allows no evaluation semantics" error.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-allowed-noexcept.C, F34.)

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct ne_only_t {
  using assertion_control_object = ne_only_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::noexcept_enforce};
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::noexcept_enforce; }
};
constexpr ne_only_t ne_only{};

// The label permits only noexcept_enforce and compute_semantic selects it; this
// must be accepted (no "allows no evaluation semantics" error).
void f(int x) pre<ne_only>(x > 0) { }
