// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t | FileCheck %s

// allowed_semantics facet: restricts which semantics are acceptable.
// When configured semantic is not in the set, it's silently adjusted.
//
// The handler prints the semantic each call was evaluated with (observe is
// 2, enforce 3), which is what the semantic transformation changes.

#include <contracts>

#include <cstdio>

// Every call below violates its precondition.  The handler prints what it
// was given and throws, so each call reports and returns, whatever its
// semantic.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("semantic %d\n", static_cast<int>(v.semantic()));
  throw 0;
}

template <class F> void call(const char *name, F f) {
  std::printf("%s: ", name);
  try {
    f();
    std::printf("no violation\n");
  } catch (int) {
  }
}
using namespace std::contracts;
using namespace std::contracts::labels;

struct observe_only_t {
  using assertion_control_object = observe_only_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set(evaluation_semantic::observe)
    | evaluation_semantic_set(evaluation_semantic::ignore);
};
constexpr observe_only_t observe_only{};

// enforce is not in allowed set -> adjusted to observe
void f(int x) pre<observe_only>(x > 0) {}

// Combined with empty: allowed_semantics propagated from observe_only side
void g(int x) pre<observe_only | empty_label>(x > 0) {}
void h(int x) pre<empty_label | observe_only>(x > 0) {}

int main() {
  call("f", [] { f(-1); });
  call("g", [] { g(-1); });
  call("h", [] { h(-1); });
}

// CHECK: f: semantic 2
// CHECK-NEXT: g: semantic 2
// CHECK-NEXT: h: semantic 2

// REQUIRES: contracts-libcxx, native
