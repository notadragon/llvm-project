// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-outside-function-body.json %libcxx_flags -o %t && %t

// P3100 checks are emitted outside a function body too: a division in a
// namespace-scope initializer, a default member initializer or a default
// argument is reported under the observe semantic and yields 0.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-outside-function-body.C

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

volatile int z = 0;
int g = 10 / z;                     // namespace-scope initializer
struct S { int m = 10 / z; };       // default member initializer
int da(int x = 10 / z) { return x; } // default argument

int main() {
  int l = 10 / z;
  S s;
  int d = da();
  if (violations != 4 || g != 0 || l != 0 || s.m != 0 || d != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
