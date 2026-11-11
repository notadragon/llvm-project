// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safetycritical:observe \
// RUN:   %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native

// P3400 [support.contract.control.combine] makes a combined label's
// group_names the sorted, unique union of its constituents' names, whatever
// their shape.  Combining labels whose group_names is an array of pointers to
// strings keeps every name whole: a name shorter than a pointer ("a") and a
// longer one ("safetycritical"), so both combined labels are in the
// safetycritical group and observed.  The merge used to copy
// sizeof(group_names[0]) characters of each name (CLANG-826): the short name
// made the combination non-constant, and the long one was cut to
// "safetycr" with no terminator and matched no group.

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
