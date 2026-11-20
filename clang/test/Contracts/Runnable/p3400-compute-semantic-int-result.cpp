// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=ignore \
// RUN:   %libcxx_flags -o %t && %t

// A label whose compute_semantic returns int is not a computed-semantics
// control type ([basic.contract.control]: the result must be convertible to
// evaluation_semantic, and int converts only explicitly; CLANG-828), so its
// compute_semantic does not take part in choosing the semantic
// ([basic.contract.eval]) and the configured ignore applies: the handler is
// not called, although the facet would turn ignore into observe.  The
// concept half is ../p3400-semantic-computation-label-convertible.cpp.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-convertible.C
// (the same rule, checked in constant evaluation).

#include <contracts>

using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct int_sem_t {
  using assertion_control_object = int_sem_t;
  constexpr int compute_semantic(evaluation_semantic) const {
    return int(evaluation_semantic::observe);
  }
};
constexpr int_sem_t int_sem{};

void f(int x) pre<int_sem>(x > 0) {}

int main() {
  f(-1);
  return violations == 0 ? 0 : 1;
}
