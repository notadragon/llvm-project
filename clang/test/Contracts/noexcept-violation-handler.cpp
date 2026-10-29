// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// A handler defined noexcept, which [basic.contract.handler] explicitly
// permits, is accepted: libc++'s <contracts> declares no handler.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/noexcept-violation-handler.C

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation &) noexcept {}

int main() {}

// REQUIRES: contracts-libcxx
