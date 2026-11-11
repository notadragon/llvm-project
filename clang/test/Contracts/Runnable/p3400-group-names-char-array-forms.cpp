// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:observe \
// RUN:   -fcontract-group-evaluation-semantic=bcd:observe %libcxx_flags -o %t
// RUN: %t

// P3400: a group name held in a character array is read whole, whatever form
// its constant value takes: a long braced list of byte values, and a trailing
// part the initializer omits (zero).  Both groups are configured to observe;
// a name read short would not match, and the violation would enforce.
//
// Mirror of gnu_gcc's p3400-group-names-char-array-forms.C (GCC-197).

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct raw_t {
  using assertion_control_object = raw_t;
  static constexpr char group_names[1][80] = {
      {97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
       97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
       97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
       97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
       97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97}};
};
constexpr raw_t raw{};

struct short_t {
  using assertion_control_object = short_t;
  static constexpr char group_names[1][8] = {{'b', 'c', 'd'}};
};
constexpr short_t short_name{};

void f(int x) pre<raw>(x > 0) {}
void g(int x) pre<short_name>(x > 0) {}

int main() {
  f(-1);
  g(-1);
  return violations == 2 ? 0 : 1;
}
