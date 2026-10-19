// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=safety:observe %libcxx_flags -o %t && %t

// P3400: group_names is read as the concept reads it -- group_names[i]
// converted to const char* -- so an array of pointers to strings, or a
// std::array, supplies the label's groups like an array of strings does.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-pointer-array.C

#include <contracts>
#include <array>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct ptr_array_t {
  using assertion_control_object = ptr_array_t;
  static constexpr const char *const group_names[] = {"safety"};
};
constexpr ptr_array_t ptr_array{};

struct std_array_t {
  using assertion_control_object = std_array_t;
  static constexpr std::array<const char *, 1> group_names = {"safety"};
};
constexpr std_array_t std_array{};

static_assert(std::contracts::labels::identification_label<ptr_array_t>);
static_assert(std::contracts::labels::identification_label<std_array_t>);

void f(int x) pre<ptr_array>(x > 0) {}
void g(int x) pre<std_array>(x > 0) {}

int main() {
  f(-1);
  g(-1);
  return violations == 2 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
