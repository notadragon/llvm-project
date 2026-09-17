// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098 x P3400 single-unit rule: under observe the capture is constructed
// (initializer side effect runs), the predicate is evaluated, the handler runs
// on failure, and execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-observe-capture.C)

#include <contracts>
#include <cstdio>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

int f() post [old = made(5)] (r: r == old) { return 7; }

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 1) __builtin_abort();
  if (violation_count != 1) __builtin_abort();
  std::printf("PASS\n");
}
