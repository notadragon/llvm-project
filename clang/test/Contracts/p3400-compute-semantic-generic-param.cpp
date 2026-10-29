// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3400: a compute_semantic facet taking `auto' is applied; the argument is an
// evaluation_semantic lvalue, so the parameter deduces to that type.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-generic-param.C

#include <contracts>

using std::contracts::evaluation_semantic;

struct generic_t {
  using assertion_control_object = generic_t;
  constexpr evaluation_semantic compute_semantic(auto s) const {
    return s == evaluation_semantic::enforce ? evaluation_semantic::observe : s;
  }
};
constexpr generic_t generic{};
static_assert(std::contracts::labels::semantic_computation_label<generic_t>);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(int x) pre<generic>(x > 0) {}

int main() {
  f(-1); // enforce -> observe: continues
  if (violations != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
