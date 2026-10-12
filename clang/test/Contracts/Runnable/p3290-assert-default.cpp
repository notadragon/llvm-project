// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: not --crash %t 2>&1 | FileCheck %s

// P3290: the assert macro is unchanged without __STDC_WANT_ASSERT_USES_CONTRACTS__.
// A failing assert is the platform's: it aborts with the platform's message
// and never reaches the contract-violation handler.
// (GCC mirror: p3290-assert-default.C)

#include <cassert>
#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::fputs("contract handler ran\n", stderr);
}

int main() {
  assert(1 == 1);
  assert(1 == 2);
  std::fputs("continued past the assert\n", stderr);
}

// CHECK-NOT: contract handler ran
// CHECK: 1 == 2
// CHECK-NOT: continued past the assert
