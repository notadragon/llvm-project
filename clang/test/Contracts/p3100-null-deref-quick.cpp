// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: null-pointer dereference (ub:expr.unary.dereference.nullptr), quick_enforce --
// the null test traps before the dereference, no handler is called.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-quick.C)

#include <contracts>
#include <csignal>
#include <cstdlib>

// quick_enforce never calls the handler: a call exits normally, which
// `not --crash` reports as a failure.
void handle_contract_violation(const std::contracts::contract_violation &) {
  std::_Exit(0);
}

int load_it(int *p) { return *p; }

int main() {
  // So does the native SIGSEGV the unchecked operation would raise.
  std::signal(SIGSEGV, [](int) { std::_Exit(0); });
  int *p = nullptr;
  return load_it(p);
}

// REQUIRES: contracts-libcxx, native
