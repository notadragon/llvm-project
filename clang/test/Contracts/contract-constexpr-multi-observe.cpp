// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only \
// RUN:   2>&1 | FileCheck %s

// In constant evaluation, every non-terminating (observe) contract violation is
// reported, not just the first.  No Clang bug found (Clang has no single-slot
// masking; GCC F30 pt2/5cdefd11880 did not reproduce).
// (GCC mirror: g++.dg/contracts/cpp26/contract-constexpr-multi-observe.C.)

#include <contracts>

// Three observe violations in one constant evaluation; all three are reported
// (each contract_assert is const-but-false) and evaluation continues.
// CHECK-COUNT-3: contract failed during execution of constexpr function
constexpr int f(int a, int b, int c) {
  contract_assert(a > 0);
  contract_assert(b > 0);
  contract_assert(c > 0);
  return a + b + c;
}

constexpr int bad = f(-1, -2, -3);
