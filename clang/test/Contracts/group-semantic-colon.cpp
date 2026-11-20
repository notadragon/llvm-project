// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe %libcxx_flags -o %t && %t

// -fcontract-group-evaluation-semantic= takes the semantic after the last
// ':', so a P3100 group, whose name always contains ':' ("ub:..."), can be
// named: the division by zero is observed and the program continues.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-group-semantic-colon.C

#include <contracts>

int violations;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

volatile int z = 0;

int main() {
  int l = 10 / z;
  if (violations != 1 || l != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
