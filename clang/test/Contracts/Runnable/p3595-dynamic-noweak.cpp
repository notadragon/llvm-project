// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-noweak.json %libcxx_flags -o %t && %t

// P3595 dynamic selection with provideweak:false.  The compiler emits NO weak
// definition of the selector, so the user must supply a strong definition (else
// the link fails).  Here the user selector returns "observe"; the program links,
// the failing precondition is handled and execution continues (violations == 1).

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

evaluation_semantic p3595_noweak_sel() {
  return evaluation_semantic::observe;
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
