// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A replacement ::handle_contract_violation defined inside an
// `extern "C++" { }' block replaces the default handler: the block does not
// change the function's scope, so the __handle_contract_violation alias is
// emitted for it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/handler-in-linkage-spec.C

#include <contracts>

static int handled = 0;

extern "C++" {
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
}
}

void f(int x) pre(x > 0) {}

int main() {
  f(-1);
  if (handled != 1)
    __builtin_abort();
}
