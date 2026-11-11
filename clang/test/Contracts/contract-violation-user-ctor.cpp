// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s

// std::contracts::contract_violation has "no user-accessible constructor"
// ([support.contract.violation]), so user code cannot fabricate a violation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-user-ctor.C

#include <contracts>

void f() {
  std::contracts::contract_violation v(nullptr); // expected-error {{private constructor}}
                                                // expected-note@*:* {{implicitly declared private here}}
}

// REQUIRES: contracts-libcxx
