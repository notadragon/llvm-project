// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safetycritical:observe \
// RUN:   %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native
// XFAIL: *

// OPEN BUG (CLANG-826 in this fork's open-issues/): libc++ accepts a label
// whose group_names is an array of pointers to strings as an
// identification label (p3400-group-names-pointer-array.cpp), but the
// group-name merge of a combined label (__group_max_len, __gn_append) is
// sizeof-based and treats each element as a character array of
// sizeof(group_names[0]) characters -- here sizeof(const char *).  Combining
// two such labels reads past the end of a shorter name ("a"), so the
// combination is not a constant expression.  A longer name ("safetycritical")
// is cut to its first sizeof(const char *) characters with no terminator, and
// the combined label is in no group at all -- configuring "safetycr" does not
// match it either -- so the contract is enforced.
// P3400 [support.contract.control.combine] makes a combined label's
// group_names the sorted, unique union of its constituents' names.

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct long_name_t {
  using assertion_control_object = long_name_t;
  static constexpr const char *const group_names[] = {"safetycritical"};
};
struct short_name_t {
  using assertion_control_object = short_name_t;
  static constexpr const char *const group_names[] = {"a"};
};

using std::contracts::labels::operator|;
using std::contracts::labels::operator""group;

constexpr auto both = long_name_t{} | short_name_t{};
constexpr auto with_group = long_name_t{} | "zz"group;

void f(int x) pre<both>(x > 0) {}
void g(int x) pre<with_group>(x > 0) {}

int main() {
  f(-1);
  g(-1);
  return violations == 2 ? 0 : 1;
}
