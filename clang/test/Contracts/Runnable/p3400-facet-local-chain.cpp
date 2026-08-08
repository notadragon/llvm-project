// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: local handler chaining with combined labels.  The RHS
// handle_contract_violation is tried first; LHS only if RHS did not handle.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-local-chain.C)

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

static int lhs_calls = 0;
static int rhs_calls = 0;
static int global_calls = 0;

struct lhs_handled_t {
  using assertion_control_object = lhs_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++lhs_calls;
    return violation_handled::handled;
  }
};
constexpr lhs_handled_t lhs_handled{};

struct rhs_handled_t {
  using assertion_control_object = rhs_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++rhs_calls;
    return violation_handled::handled;
  }
};
constexpr rhs_handled_t rhs_handled{};

struct rhs_not_handled_t {
  using assertion_control_object = rhs_not_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++rhs_calls;
    return violation_handled::not_handled;
  }
};
constexpr rhs_not_handled_t rhs_not_handled{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

void case_both_handle(int x) pre<(lhs_handled | rhs_handled)>(x > 0) { }
void case_lhs_not_rhs_yes(int x) pre<(rhs_not_handled | rhs_handled)>(x > 0) { }
void case_only_lhs(int x) pre<(lhs_handled | empty_label)>(x > 0) { }
void case_only_rhs(int x) pre<(empty_label | rhs_handled)>(x > 0) { }
void case_neither(int x) pre<(empty_label | empty_label)>(x > 0) { }

int main() {
  lhs_calls = rhs_calls = global_calls = 0;
  case_both_handle(-1);
  if (lhs_calls != 1 || rhs_calls != 0 || global_calls != 0) __builtin_abort();

  lhs_calls = rhs_calls = global_calls = 0;
  case_lhs_not_rhs_yes(-1);
  if (rhs_calls != 2 || global_calls != 0) __builtin_abort();

  lhs_calls = rhs_calls = global_calls = 0;
  case_only_lhs(-1);
  if (lhs_calls != 1 || global_calls != 0) __builtin_abort();

  lhs_calls = rhs_calls = global_calls = 0;
  case_only_rhs(-1);
  if (rhs_calls != 1 || global_calls != 0) __builtin_abort();

  lhs_calls = rhs_calls = global_calls = 0;
  case_neither(-1);
  if (lhs_calls != 0 && rhs_calls != 0 || global_calls != 1) __builtin_abort();
}
