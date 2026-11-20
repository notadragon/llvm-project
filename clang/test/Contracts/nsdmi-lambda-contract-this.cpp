// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A this-capturing lambda whose precondition names a member, in a default
// member initializer of a class template: the precondition reads the member
// of the object being initialized.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/nsdmi-lambda-contract-this.C

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
}

template <class T> struct S {
  T m = 5;
  bool r = [this](int x) pre(x > m) { return true; }(1);   // 1 > 5 fails
  bool q = [this](int x) pre(x > m) { return true; }(9);   // holds
};

int main() {
  S<int> s;
  if (!s.r || !s.q || handled != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
