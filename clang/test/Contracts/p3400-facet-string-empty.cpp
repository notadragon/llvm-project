// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: a compute_comment / compute_message facet returning "" (a
// redacting facet) is applied: an empty result is a result.
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

struct empty_t {
  using assertion_control_object = empty_t;
  constexpr const char *compute_comment(const char *) const { return ""; }
  constexpr const char *compute_message(const char *) const { return ""; }
};
constexpr empty_t empty{};

void f(int x) pre<empty>(x > 0, "secret") {}

int main() {
  f(0);
  if (!is(comment, "") || !is(message, ""))
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
