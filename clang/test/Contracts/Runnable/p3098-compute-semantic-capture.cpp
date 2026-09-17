// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3098 x P3400 single-unit rule with compute_semantic: the 'review' label
// (enforce -> observe) keeps the capture live -- constructed once, predicate
// evaluated, reported under observe, execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-compute-semantic-capture.C)

#include <contracts>
#include <cstdio>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

int f() post<review> [old = made(5)] (r: r == old) { return 7; }

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 1) __builtin_abort();
  if (violation_count != 1) __builtin_abort();
  std::printf("PASS\n");
}
