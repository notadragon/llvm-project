// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe \
// RUN:   -fcontract-group-evaluation-semantic=audit:observe %libcxx_flags -o %t
// RUN: %t

// P3400: group identification labels combined with operator|.  A combined
// label should inherit group_names from both sides.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-group-combined.C)
//
// (Previously: operator|-combined labels did not propagate group_names --
// extractGroupNames only looked for the group_names member as a direct field, but
// a combined label carries it in the __combined_identification_label base, so the
// safety/audit groups resolved to the default.  extractGroupNames now reads
// group_names from the owning base subobject.)

#include <contracts>

using std::contracts::labels::operator|;
using std::contracts::labels::review;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void f_safety_audit(int x) pre<("safety"group | "audit"group)>(x > 0) { }
void f_safety_review(int x) pre<("safety"group | review)>(x > 0) { }
void f_review_safety(int x) pre<(review | "safety"group)>(x > 0) { }
void f_review_review(int x) pre<(review | review)>(x > 0) { }
void f_audit_other(int x) pre<("audit"group | "other"group)>(x > 0) { }
void f_other_audit(int x) pre<("other"group | "audit"group)>(x > 0) { }

int main() {
  f_safety_audit(-1);
  if (violations != 1) __builtin_abort();
  f_safety_review(-1);
  if (violations != 2) __builtin_abort();
  f_review_safety(-1);
  if (violations != 3) __builtin_abort();
  f_review_review(-1);
  if (violations != 4) __builtin_abort();
  f_audit_other(-1);
  if (violations != 5) __builtin_abort();
  f_other_audit(-1);
  if (violations != 6) __builtin_abort();
}
