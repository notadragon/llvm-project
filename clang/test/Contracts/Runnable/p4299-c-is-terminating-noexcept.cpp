// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=noexcept_enforce %libcxx_flags -o %t
// RUN: %t

// The C accessor stdc_contract_violation_is_terminating must agree with the C++
// contract_violation::is_terminating() member over the identical data block: a
// noexcept_enforce (P4298) violation is terminating, and a C handler inspecting
// it through libcontracts must see that.  C-side mirror of the P4298
// is_terminating classification fix.
// (GCC mirror: g++.dg/contracts/cpp26/p4299-c-is-terminating-noexcept.C.)

#include <contracts>
#include <cstdlib>

extern "C" int stdc_contract_violation_is_terminating(const void *);

void handle_contract_violation(const std::contracts::contract_violation &v) {
  // A contract_violation is layout-compatible with the C view (a single leading
  // data-block chain pointer), so the C accessor reads it directly.
  int terminating = stdc_contract_violation_is_terminating((const void *)&v);
  if (terminating == 1)
    std::exit(0);        // correct: noexcept_enforce is terminating
  __builtin_abort();     // wrong classification
}

int f(int x) pre(x > 0) { return x; }

int main() {
  f(-1);              // violates the precondition -> handler runs, then exits
  __builtin_abort();  // must not be reached
}
