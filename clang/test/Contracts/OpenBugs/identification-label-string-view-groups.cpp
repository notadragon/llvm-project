// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe \
// RUN:   %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native
// XFAIL: *

// OPEN BUG (CLANG-826 in this fork's open-issues/): a label whose
// group_names is a range of std::string_view is an identification label
// (P3400 [support.contract.control.ident]), so its groups select the
// contract's configuration.  Sema's extractGroupNames converts each
// group_names[i] to const char*, which a std::string_view does not, so it
// finds no groups; the precondition is configured as unlabelled, enforced,
// and the program aborts instead of observing the violation.  The
// concept half is identification-label-string-view-range.cpp.

#include <array>
#include <contracts>
#include <string_view>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct sv_array_t {
  using assertion_control_object = sv_array_t;
  static constexpr std::array<std::string_view, 1> group_names = {"safety"};
};
constexpr sv_array_t sv_array{};

void f(int x) pre<sv_array>(x > 0) {}

int main() {
  f(-1);
  return violations == 1 ? 0 : 1;
}
