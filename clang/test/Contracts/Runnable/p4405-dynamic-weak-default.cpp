// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-dynamic-weak-default.json %libcxx_flags -o %t -Xclang -verify
// RUN: %t
// expected-no-diagnostics

// P4405: a dynamically selected contract whose configuration names prove as
// the default semantic gets a weak default selector that returns prove,
// which behaves as ignore at run time: the predicate is not evaluated and
// nothing is reported.  Nothing is diagnosed at compile time either.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-dynamic-weak-default.C

#include <contracts>

extern "C" std::contracts::evaluation_semantic p4405_weak_prove_sel();

int violations, evaluated;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

bool chk(int x) { ++evaluated; return x > 0; }

int f(int x) pre(chk(x)) { return x; }

int main() {
  if (p4405_weak_prove_sel() != std::contracts::evaluation_semantic::prove)
    __builtin_abort();
  f(-1);
  return violations + evaluated;
}
