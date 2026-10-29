// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A contract_assert inside a GNU statement expression in another
// contract_assert's predicate compiles and runs: the nested assertion is
// checked, and reported once when it fails.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/nested-assert-in-stmt-expr.C

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
}

int f(int x) {
  contract_assert(__extension__({ contract_assert(x > 1); x > 0; }));
  return x;
}

int main() {
  f(2);
  if (handled != 0)
    __builtin_abort();
  f(1);   // the nested assertion fails, the outer one holds
  if (handled != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
