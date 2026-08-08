// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce -fsyntax-only

// review label transforms enforce→observe.

#include <contracts>

struct always_observe_t {
  using assertion_control_object = always_observe_t;
  constexpr std::contracts::evaluation_semantic
  compute_semantic(std::contracts::evaluation_semantic) const {
    return std::contracts::evaluation_semantic::observe;
  }
};
constexpr always_observe_t always_observe{};

// These should compile — review and always_observe transform semantics
void f(int x) pre<review>(x > 0) {}
void g(int x) pre<always_observe>(x > 0) {}

// Empty label has no compute_semantic — semantic unchanged
void h(int x) pre<empty_label>(x > 0) {}
