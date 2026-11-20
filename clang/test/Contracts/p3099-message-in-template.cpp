// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3099: a diagnostic message in a template -- type-dependent, or a
// non-dependent class type -- is accepted and delivered to the handler.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3099-message-template-dependent.C

#include <contracts>
#include <cstring>
#include <string_view>

static int good = 0, seen = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++seen;
  const char *m = v.message();
  if (m && (!std::strcmp(m, "abc") || !std::strcmp(m, "d")))
    ++good;
}

struct M1 {
  static constexpr int size() { return 3; }
  static constexpr const char *data() { return "abc"; }
};
struct M {
  constexpr std::size_t size() const { return 3; }
  constexpr const char *data() const { return "abc"; }
};

template <class T> void f(int x) pre(x > 0, T{}) {}
template <class T> void g(int x) { contract_assert(x > 0, T{}); }
template <class T> constexpr int h(T x) pre(x > 0, M{}) { return 1; }
template <class T> void k(T x) { contract_assert(x > 0, std::string_view("d")); }

int main() {
  f<M1>(-1);
  g<M1>(-1);
  h(-1);
  k(-1);
  return (seen == 4 && good == 4) ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
