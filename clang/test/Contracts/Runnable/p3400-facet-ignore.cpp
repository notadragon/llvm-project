// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t

// P3400: compute_semantic with ignore as the configured semantic.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-ignore.C)

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

static int violations = 0;
static bool should_not_be_called = false;

void handle_contract_violation(const std::contracts::contract_violation&) {
  if (should_not_be_called)
    __builtin_abort();
  ++violations;
}

// review: ignore -> ignore (review only transforms enforce/quick_enforce).
void with_review(int x) pre<review>(x > 0) { }
// always_observe: ignore -> observe (overrides to observe).
void with_always_observe(int x) pre<always_observe>(x > 0) { }
// always_observe | review: ignore -> observe -> observe.
void observe_then_review(int x)
  pre<(always_observe | review)>(x > 0)
{
}

int main() {
  should_not_be_called = true;
  with_review(-1);  // ignore: no evaluation, no handler
  should_not_be_called = false;
  if (violations != 0) __builtin_abort();

  with_always_observe(-1);
  if (violations != 1) __builtin_abort();

  observe_then_review(-1);
  if (violations != 2) __builtin_abort();
}
