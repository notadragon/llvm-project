// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fsyntax-only 2>&1 | FileCheck %s

// P3400: allowed_semantics facet -- error cases.  An empty allowed set, and a
// compute_semantic result outside the allowed set, are both rejected.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-allowed-error.C)

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::labels::operator|;

// Empty allowed set.
struct nothing_allowed_t {
  using assertion_control_object = nothing_allowed_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set::none();
};
constexpr nothing_allowed_t nothing{};

// CHECK: allows no evaluation semantics
void f(int x) pre<nothing>(x > 0) { }

// Allowed only observe, but compute_semantic returns enforce.
struct bad_compute_t {
  using assertion_control_object = bad_compute_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe};
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::enforce; }
};
constexpr bad_compute_t bad_compute{};

// CHECK: compute_semantic result is not in the allowed
void g(int x) pre<bad_compute>(x > 0) { }

// Two labels whose allowed_semantics are disjoint.  Combining them with
// operator| intersects the sets to the empty set, which is ill-formed for the
// same reason as the explicitly-empty set above (the empty-intersection case).
struct observe_or_ignore_t {
  using assertion_control_object = observe_or_ignore_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::ignore};
};
constexpr observe_or_ignore_t observe_or_ignore{};

struct terminating_t {
  using assertion_control_object = terminating_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::enforce, evaluation_semantic::quick_enforce};
};
constexpr terminating_t terminating{};

// CHECK: allows no evaluation semantics
void h(int x) pre<(terminating | observe_or_ignore)>(x > 0) { }
