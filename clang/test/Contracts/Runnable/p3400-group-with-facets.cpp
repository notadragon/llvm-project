// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe %libcxx_flags -o %t
// RUN: %t

// P3400: group labels combined with other facets (allowed_semantics,
// compute_comment).
// (GCC mirror: g++.dg/contracts/cpp26/p3400-group-with-facets.C)
//
// (Previously: operator|-combined labels did not propagate group_names,
// so the "safety" group config never applied to these combined labels.
// extractGroupNames now reads group_names from the __combined_identification_label
// base subobject, including nested combinations.)

#include <contracts>
#include <cstring>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::labels::operator|;

struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe};
};
constexpr only_observe_t only_observe{};

struct redact_comment_t {
  using assertion_control_object = redact_comment_t;
  constexpr const char* compute_comment(const char*) const {
    return "[redacted]";
  }
};
constexpr redact_comment_t redact_comment{};

static int violations = 0;
static const char* last_comment = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++violations;
  last_comment = v.comment();
}

void f_group_allowed(int x) pre<("safety"group | only_observe)>(x > 0) { }
void f_group_comment(int x) pre<("safety"group | redact_comment)>(x > 0) { }
void f_triple(int x)
  pre<("safety"group | only_observe | redact_comment)>(x > 0) { }

int main() {
  f_group_allowed(-1);
  if (violations != 1) __builtin_abort();

  f_group_comment(-1);
  if (violations != 2) __builtin_abort();
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();

  f_triple(-1);
  if (violations != 3) __builtin_abort();
  if (!last_comment || std::strcmp(last_comment, "[redacted]") != 0)
    __builtin_abort();
}
