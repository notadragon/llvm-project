// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=enforce -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A repeated -fcontract-evaluation-semantic= is accepted and the last one
// wins, as for any other option: enforce, then observe, observes.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/evaluation-semantic-last-wins.C

#include <contracts>

int violations;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x) pre(x > 0) { return x; }

int main() {
  f(-1);
  if (violations != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
