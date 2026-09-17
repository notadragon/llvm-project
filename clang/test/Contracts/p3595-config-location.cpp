// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-config-location.json %libcxx_flags -o %t && %t

// P3595: test location matching (filename suffix + line ranges) in JSON config.
// Config sets lines 20-23 of this file to ignore, caller to ignore,
// everything else to observe.

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// This function's contract is outside lines 20-23 -> observe
void f_outside(int x)
  pre(x > 0)
{
}
// This function and its contract (lines 20-23) are in the ignore range.
void f_ignored(int x)
  pre(x > 0)
{
}

int main() {
  // f_ignored's contract is in the ignore range: no handler call
  f_ignored(-1);
  if (violations != 0) { printf("FAIL: expected 0, got %d\n", violations); abort(); }

  // f_outside's contract is outside the range: observe -> handler called
  f_outside(-1);
  if (violations != 1) { printf("FAIL: expected 1, got %d\n", violations); abort(); }

  printf("PASS\n");
}
