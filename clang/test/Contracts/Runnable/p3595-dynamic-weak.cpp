// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-weak.json %libcxx_flags -o %t && %t

// P3595 dynamic selection: with provideweak (default true) and no user-supplied
// selector, the compiler emits a weak definition of the selector that returns
// the compile-time default semantic ("observe").  The program links and runs;
// the failing precondition is handled (observe) and execution continues.

#include <contracts>
#include <cstdlib>

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(const int x) pre(x > 0) {}

int main() {
  f(-1);
  if (violations != 1)
    std::abort();
  f(1);
  if (violations != 1)
    std::abort();
}
