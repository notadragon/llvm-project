// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s

// A deleted ::handle_contract_violation is rejected (treated as if
// [dcl.fct.def.replace] said the handler shall not be deleted), with an error
// naming the handler.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/deleted-violation-handler.C
// (GCC: "'::handle_contract_violation' cannot be deleted").

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation &) = delete;
// expected-error@-1 {{'::handle_contract_violation' must not be deleted}}

int main() {}

// REQUIRES: contracts-libcxx
