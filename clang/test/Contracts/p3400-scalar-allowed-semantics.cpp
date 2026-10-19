// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3400: an allowed_semantics facet is read as evaluation_semantic_set
// ({label.allowed_semantics}), as in the concept, so a label holding a single
// evaluation_semantic restricts the semantic to that one.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-scalar-allowed-semantics.C

#include <contracts>

using std::contracts::evaluation_semantic;

struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic allowed_semantics =
      evaluation_semantic::observe;
};
constexpr only_observe_t only_observe{};
static_assert(std::contracts::labels::allowed_semantics_label<only_observe_t>);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(int x) pre<only_observe>(x > 0) {}

int main() {
  f(-1); // clamped to observe: continues
  if (violations != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
