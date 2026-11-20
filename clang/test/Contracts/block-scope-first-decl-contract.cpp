// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A block-scope function declaration that is the first declaration carries a
// precondition, and the namespace-scope definition omits it (which is
// allowed): the call is checked.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/block-scope-first-decl-contract.C

#include <contracts>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

int main() {
  void lx(int x) pre(x > 0);
  lx(-1);
  if (reported != 1)
    __builtin_abort();
}

void lx(int) {}

// REQUIRES: contracts-libcxx, native
