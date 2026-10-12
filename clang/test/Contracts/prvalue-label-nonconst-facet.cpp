// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// The labels:: concepts probe a const object, so a non-const member function
// is not a facet, for a prvalue label `pre<L{}>' as for an lvalue one: only
// the global handler runs, with the original comment.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/prvalue-label-nonconst-facet.C

#include <contracts>
#include <cstring>

static int local_calls = 0, global_calls = 0;
static const char *comment;

struct L {
  using assertion_control_object = L;
  void handle_contract_violation(const std::contracts::contract_violation &) {
    ++local_calls;
  }
  constexpr const char *compute_comment(const char *) { return "nonconst"; }
};

static_assert(!std::contracts::labels::local_violation_label<L>);
static_assert(!std::contracts::labels::compute_comment_label<L>);

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++global_calls;
  comment = v.comment();
}

void f(int x) pre<L{}>(x > 0) {}

int main() {
  f(0);
  if (local_calls != 0 || global_calls != 1 || !comment ||
      std::strcmp(comment, "x > 0") != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
