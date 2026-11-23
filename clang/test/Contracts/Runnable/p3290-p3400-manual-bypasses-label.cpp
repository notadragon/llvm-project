// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -fcontracts-p3400 \
// RUN:   -fcontracts-p3099 -fcontract-evaluation-semantic=observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P3290 x P3400: a violation raised through the P3290 API is not a contract
// assertion, so no assertion-control object takes part in it.  It reaches
// the global handler directly, with kind manual and the comment and message
// it was given, even when it is raised inside a function, or inside a
// predicate, whose contract assertions carry a label with every
// handler-time facet: a local violation handler that handles everything, a
// compute_comment and a compute_message.  The labelled assertions in the same
// functions still go through the label.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3290-p3400-manual-bypasses-label.C

#include <contracts>
#include <cstring>

using namespace std::contracts;

static int local_calls, global_calls;
static assertion_kind last_kind;
static const char *last_comment, *last_message;

struct everything_t {
  using assertion_control_object = everything_t;
  violation_handled
  handle_contract_violation(const contract_violation &) const {
    ++local_calls;
    return violation_handled::handled;
  }
  constexpr const char *compute_comment(const char *) const {
    return "label comment";
  }
  constexpr const char *compute_message(const char *) const {
    return "label message";
  }
};
constexpr everything_t everything{};

void handle_contract_violation(const contract_violation &v) {
  ++global_calls;
  last_kind = v.kind();
  last_comment = v.comment();
  last_message = v.message();
}

void body(int x) pre<everything>(x > 0) {
  contract_assert<everything>(x > 1);
  handle_observed_contract_violation("manual");
}

static bool raise() {
  handle_observed_contract_violation("from predicate");
  return true;
}

void pred() pre<everything>(raise()) {}

int main() {
  body(0); // pre and contract_assert fail: both handled by the label
  if (local_calls != 2 || global_calls != 1)
    __builtin_abort();
  if (last_kind != assertion_kind::manual ||
      std::strcmp(last_comment, "manual") != 0 || last_message != nullptr)
    __builtin_abort();

  pred(); // the predicate holds; the manual violation in it is global
  if (local_calls != 2 || global_calls != 2)
    __builtin_abort();
  if (last_kind != assertion_kind::manual ||
      std::strcmp(last_comment, "from predicate") != 0 ||
      last_message != nullptr)
    __builtin_abort();
}
