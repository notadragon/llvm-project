// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=noexcept_enforce %libcxx_flags -o %t
// RUN: not --crash %t 2>&1 | FileCheck %s

// D4298: contract_violation::is_terminating() must report true for
// noexcept_enforce -- it aborts on a normally-returning handler exactly like
// enforce.  Regression: is_terminating() checked only enforce/quick_enforce and
// returned false for noexcept_enforce.
// (GCC mirror: g++.dg/contracts/cpp26/p4298-is-terminating.C.)

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (v.semantic() != std::contracts::evaluation_semantic::noexcept_enforce)
    return;  // wrong semantic -> no marker -> FileCheck fails
  if (v.is_terminating()) {
    std::fputs("IS_TERMINATING\n", stderr);
    std::fflush(stderr);
  }
  // Returns normally -> noexcept_enforce terminates.
}

// CHECK: IS_TERMINATING
int f(int x) pre(x > 0) { return x; }

int main() { f(-1); return 0; }
