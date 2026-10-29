// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t | FileCheck %s

// review label transforms enforce->observe.
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

struct always_observe_t {
  using assertion_control_object = always_observe_t;
  constexpr std::contracts::evaluation_semantic
  compute_semantic(std::contracts::evaluation_semantic) const {
    return std::contracts::evaluation_semantic::observe;
  }
};
constexpr always_observe_t fixed_observe{};

// These should compile -- review and fixed_observe transform semantics
void f(int x) pre<review>(x > 0) {}
void g(int x) pre<fixed_observe>(x > 0) {}

// Empty label has no compute_semantic -- semantic unchanged
void h(int x) pre<empty_label>(x > 0) {}

int main() {
  call("f", [] { f(-1); });
  call("g", [] { g(-1); });
  call("h", [] { h(-1); });
}

// CHECK: f: semantic 2
// CHECK-NEXT: g: semantic 2
// CHECK-NEXT: h: semantic 3

// REQUIRES: contracts-libcxx, native
