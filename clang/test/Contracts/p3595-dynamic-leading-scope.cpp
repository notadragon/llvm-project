// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-leading-scope.json %libcxx_flags -o %t && %t

// A P3595 "C++"-linkage selector named "::my_sel": the leading "::" names the
// global namespace, so the check calls my_sel(), which says observe: the
// violation is reported and execution continues.  Other empty components
// are rejected (p3595-dynamic-bad-name.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-leading-scope.C

#include <contracts>

std::contracts::evaluation_semantic my_sel() {
  return std::contracts::evaluation_semantic::observe;
}

int violations;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(int x) pre(x > 0) {}

int main() {
  f(-1);
  if (violations != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
