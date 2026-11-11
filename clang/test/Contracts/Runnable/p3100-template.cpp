// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-allow-assume \
// RUN:   -fcontract-evaluation-semantic=assume %libcxx_flags -o %t
// RUN: %t

// P3100 x templates: the assume semantic on a function template with a dependent
// predicate.  assume lowers to ignore, so the predicate is not evaluated for any
// instantiation.  (GCC mirror: p3100-template.C)

#include <contracts>
#include <cstdio>

static int side = 0;
template <typename T>
bool pos(T x) { ++side; return x > T{}; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename T>
T f(T x) pre(pos(x)) { return x; }

int main() {
  side = 0;
  violation_count = 0;
  if (f(-1) != -1) __builtin_abort();
  if (f(-1.0) != -1.0) __builtin_abort();
  if (side != 0) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
  std::printf("PASS\n");
}
