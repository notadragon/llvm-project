// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-caller-side-run.json %libcxx_flags -o %t -Xclang -verify
// RUN: %t

// P4405: a caller-side diagnose has no run-time effect: its predicate is not
// evaluated at the call, and it is diagnosed once, at the call.  Another
// contract of the callee is still checked (here a callee-side observed
// postcondition).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-caller-side-run.C (whose
// observed postcondition is caller-side and so makes GCC build a wrapper at
// the call; Clang does not implement caller-side checks, so it is
// callee-side here)

#include <contracts>

static int pre_evaluated, post_evaluated, violations;
bool pre_chk(int x) { ++pre_evaluated; return x > 0; }
bool post_chk(int r) { ++post_evaluated; return r > 0; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x) pre(pre_chk(x)) post(r: post_chk(r)) { return x; } // expected-note {{the predicate of the contract assertion}}

int main() {
  f(-1); // expected-warning {{contract assertion cannot be proven}}
  if (pre_evaluated != 0 || post_evaluated != 1 || violations != 1)
    __builtin_abort();
}
