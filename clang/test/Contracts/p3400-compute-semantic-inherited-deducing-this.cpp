// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3400: a compute_semantic facet that is a deducing-this member template
// inherited from a base is applied.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-inherited-deducing-this.C

#include <contracts>

using std::contracts::evaluation_semantic;

struct compute_base {
  template <class Self>
  constexpr evaluation_semantic compute_semantic(this const Self &,
                                                 evaluation_semantic s) {
    return s == evaluation_semantic::enforce ? evaluation_semantic::observe : s;
  }
};
struct derived_t : compute_base {
  using assertion_control_object = derived_t;
};
constexpr derived_t derived{};
static_assert(std::contracts::labels::semantic_computation_label<derived_t>);
static_assert(derived.compute_semantic(evaluation_semantic::enforce) ==
              evaluation_semantic::observe);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(int x) pre<derived>(x > 0) {}

int main() {
  f(-1); // enforce -> observe: continues
  if (violations != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
