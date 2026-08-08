// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3400: compute_semantic chaining order (left-to-right).  The configured
// enforce is transformed to observe by the chain, so execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-chain-order.C)

#include <contracts>

using std::contracts::labels::operator|;
using std::contracts::evaluation_semantic;

template<evaluation_semantic _S>
struct fixed_semantic_t {
  using assertion_control_object = fixed_semantic_t;
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const { return _S; }
};

constexpr fixed_semantic_t<evaluation_semantic::enforce> always_enforce{};
constexpr fixed_semantic_t<evaluation_semantic::observe> always_observe{};

// always_enforce | review: enforce -> enforce -> observe.  Continues.
void enforce_then_review(int x)
  pre<(always_enforce | review)>(x > 0)
{
}

// always_enforce | always_observe: any -> enforce -> observe.  Continues.
void enforce_then_observe(int x)
  pre<(always_enforce | always_observe)>(x > 0)
{
}

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  enforce_then_review(-1);
  if (violations != 1) __builtin_abort();
  enforce_then_observe(-1);
  if (violations != 2) __builtin_abort();
}
