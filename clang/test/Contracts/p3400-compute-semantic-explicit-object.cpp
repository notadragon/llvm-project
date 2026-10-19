// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3400: a compute_semantic facet declared with an explicit object parameter
// is applied: the facet is called with an evaluation_semantic lvalue, as the
// concept does, and overload resolution handles the object parameter.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-explicit-object.C

#include <contracts>

using std::contracts::evaluation_semantic;

struct xobj_t {
  using assertion_control_object = xobj_t;
  constexpr evaluation_semantic compute_semantic(this const xobj_t &,
                                                 evaluation_semantic s) {
    return s == evaluation_semantic::enforce ? evaluation_semantic::observe : s;
  }
};
constexpr xobj_t xobj{};
static_assert(std::contracts::labels::semantic_computation_label<xobj_t>);
static_assert(xobj.compute_semantic(evaluation_semantic::enforce) ==
              evaluation_semantic::observe);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(int x) pre<xobj>(x > 0) {}

int main() {
  f(-1); // enforce -> observe: continues
  if (violations != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
