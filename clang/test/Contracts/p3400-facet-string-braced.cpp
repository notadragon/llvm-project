// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: a compute_comment / compute_message facet returning a braced array
// is applied.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-string-braced-empty.C

#include <contracts>
#include <cstring>

static const char *comment, *message;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  comment = v.comment();
  message = v.message();
}

static bool is(const char *p, const char *s) { return p && !std::strcmp(p, s); }

struct braced_t {
  using assertion_control_object = braced_t;
  static constexpr char txt[] = {'b', 'r', 'a', 'c', 'e', 'd', '\0'};
  constexpr const char *compute_comment(const char *) const { return txt; }
  constexpr const char *compute_message(const char *) const { return txt; }
};
constexpr braced_t braced{};

void f(int x) pre<braced>(x > 0, "m") {}

int main() {
  f(0);
  if (!is(comment, "braced") || !is(message, "braced"))
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
