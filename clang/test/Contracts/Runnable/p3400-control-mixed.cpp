// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: mixed normal and `using contract_control namespace` directives in
// lookup.  (GCC mirror: g++.dg/contracts/cpp26/p3400-control-mixed.C)

#include <contracts>

namespace labels_a {
  struct label_a_t { using assertion_control_object = label_a_t; };
  constexpr label_a_t label_a{};
}

namespace labels_b {
  struct label_b_t { using assertion_control_object = label_b_t; };
  constexpr label_b_t label_b{};
}

namespace utilities {
  constexpr int magic = 42;
}

// Normal using directive -- makes utilities::magic visible everywhere.
using namespace utilities;

// Contract-control directives -- make labels visible only in assertions.
using contract_control namespace labels_a;
using contract_control namespace labels_b;

// Both labels found through contract_control, magic through normal using.
void f(int x)
  pre<label_a>(x > 0)
  pre<label_b>(x < magic)
{
}

int get_magic() { return magic; }

constexpr auto copy_a = contract_control(label_a);
constexpr auto copy_b = contract_control(label_b);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  f(-1);      // violates first pre (label_a)
  if (violations != 1) __builtin_abort();
  f(100);     // violates second pre (label_b, x < 42)
  if (violations != 2) __builtin_abort();
  f(10);      // passes both
  if (get_magic() != 42) __builtin_abort();
}
