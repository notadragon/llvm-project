// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce -fsyntax-only

// All permutations: neither/allowed-only/compute-only/both/combined.

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

// Neither facet
void f1(int x) pre<empty_label>(x > 0) {}

// Allowed-only
struct allowed_t {
  using assertion_control_object = allowed_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::enforce)
    | evaluation_semantic_set(evaluation_semantic::observe);
};
constexpr allowed_t allowed{};
void f2(int x) pre<allowed>(x > 0) {}

// Compute-only
void f3(int x) pre<review>(x > 0) {}

// Both
struct reviewed_allowed_t {
  using assertion_control_object = reviewed_allowed_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::observe)
    | evaluation_semantic_set(evaluation_semantic::ignore);
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic __s) const {
    return evaluation_semantic::observe;
  }
};
constexpr reviewed_allowed_t reviewed_allowed{};
void f4(int x) pre<reviewed_allowed>(x > 0) {}

// Combined
void f5(int x) pre<review | allowed>(x > 0) {}
void f6(int x) pre<allowed | empty_label>(x > 0) {}
