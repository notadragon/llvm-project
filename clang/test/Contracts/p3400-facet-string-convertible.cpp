// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: a compute_comment / compute_message facet whose result is a class
// type convertible to const char* is applied: the result is converted as the
// concept requires.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-string-convertible.C

#include <contracts>
#include <cstring>

static const char *comment, *message;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  comment = v.comment();
  message = v.message();
}

static bool is(const char *p, const char *s) { return p && !std::strcmp(p, s); }

struct cstr {
  const char *p;
  constexpr operator const char *() const { return p; }
};
struct conv_t {
  using assertion_control_object = conv_t;
  constexpr cstr compute_comment(const char *) const { return {"conv"}; }
  constexpr cstr compute_message(const char *) const { return {"conv"}; }
};
constexpr conv_t conv{};

void f(int x) pre<conv>(x > 0, "m") {}

int main() {
  f(0);
  if (!is(comment, "conv") || !is(message, "conv"))
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
