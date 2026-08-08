// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce -fsyntax-only

// allowed_semantics facet: restricts which semantics are acceptable.
// When configured semantic is not in the set, it's silently adjusted.

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

struct observe_only_t {
  using assertion_control_object = observe_only_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::observe)
    | evaluation_semantic_set(evaluation_semantic::ignore);
};
constexpr observe_only_t observe_only{};

// enforce is not in allowed set → adjusted to observe
void f(int x) pre<observe_only>(x > 0) {}

// Combined with empty: allowed_semantics propagated from observe_only side
void g(int x) pre<observe_only | empty_label>(x > 0) {}
void h(int x) pre<empty_label | observe_only>(x > 0) {}
