// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t

// P3098 x P3400 single-unit rule: under ignore, a capture is not constructed
// (its initializer side effect does not run) and the predicate is not evaluated.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-ignore-suppresses-capture.C)

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
  if (init_count != 0) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
  std::printf("PASS\n");
}
