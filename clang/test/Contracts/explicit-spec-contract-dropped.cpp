// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// An explicit specialization declared with a contract and defined without
// one keeps the contract: the definition inherits it from the
// specialization's own earlier declaration (not from a declaration the
// template's instantiation made).
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/explicit-spec-contract-dropped.C

#include <contracts>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

template <class T> void ft(T x);
template <> void ft<int>(int x) pre(x > 5);
template <> void ft<int>(int x) {}

int main() {
  ft(1);
  if (reported != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
