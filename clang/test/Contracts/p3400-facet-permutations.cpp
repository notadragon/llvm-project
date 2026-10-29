// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t | FileCheck %s

// All permutations: neither/allowed-only/compute-only/both/combined.
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

int main() {
  call("f1", [] { f1(-1); });
  call("f2", [] { f2(-1); });
  call("f3", [] { f3(-1); });
  call("f4", [] { f4(-1); });
  call("f5", [] { f5(-1); });
  call("f6", [] { f6(-1); });
}

// CHECK: f1: semantic 3
// CHECK-NEXT: f2: semantic 3
// CHECK-NEXT: f3: semantic 2
// CHECK-NEXT: f4: semantic 2
// CHECK-NEXT: f5: semantic 2
// CHECK-NEXT: f6: semantic 3

// REQUIRES: contracts-libcxx, native
