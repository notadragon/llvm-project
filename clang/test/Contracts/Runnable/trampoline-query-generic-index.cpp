// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// The runtime calls a label's facets through a trampoline that calls them as
// C++ would, with overload resolution, so every shape the P3400 label
// concepts admit works.  Here: a label query that is an abbreviated function template (`auto idx').  The facet must run once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/trampoline-facet-shapes.C
// (all six shapes).

#include <contracts>

using std::contracts::contract_violation;

static int local = 0;
static int key;

struct lab_t {
  using assertion_control_object = lab_t;
  void *query(const void *k, auto idx) const { ++local; return k == &key && idx == 0 ? &key : nullptr; }
};
constexpr lab_t lab{};

void f(int x) pre<lab>(x > 0) {}

void handle_contract_violation(const contract_violation &v) {
  (void)v.query_control_object(&key, 0);
}

int main() {
  f(-1);
  if (local != 1)
    __builtin_abort();
}
