// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3098 \
// RUN:   -fcontract-group-evaluation-semantic=g:ignore \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3098 x P3400 x P3595 single-unit rule via group configuration: a group
// configured to ignore suppresses capture construction and predicate evaluation
// even though the base semantic is enforce.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-group-ignore-capture.C)

#include <contracts>
#include <cstdio>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

int f() post<"g"group> [old = made(5)] (r: r == old) { return 7; }

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 0) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
  std::printf("PASS\n");
}
