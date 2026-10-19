// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A `query' member returning `const void*' is not the queryable facet
// (labels::queryable_label requires the result to be exactly `void*'), so
// query_control_object finds nothing.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/query-const-void-return.C

#include <contracts>

static int found = 0, violations = 0;

struct L {
  using assertion_control_object = L;
  static constexpr int key = 1;
  const void *query(const void *k, std::size_t) const {
    return k == &key ? "found" : nullptr;
  }
};
static_assert(!std::contracts::labels::queryable_label<L>);
constexpr L lab{};

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  if (v.query_control_object(&L::key))
    ++found;
}

void f(int x) pre<lab>(x > 0) {}

int main() {
  f(0);
  if (violations != 1 || found != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
