// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -fcontracts-p4298 %libcxx_flags -o %t && %t

// Under -fcontracts-p4298, std::contracts::handle_enforced_contract_violation
// (std::nothrow, ...) reports noexcept_enforce, and a handler that returns is
// followed by abort, as for enforce: the [[noreturn]] function does not
// return.  A SIGABRT handler turns the abort into success.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4298-nothrow-enforce-returns.C

#include <contracts>
#include <csignal>
#include <cstdlib>
#include <new>

void handle_contract_violation(const std::contracts::contract_violation &) {}

extern "C" void on_abort(int) { std::_Exit(0); }

__attribute__((noinline)) void g() {
  std::contracts::handle_enforced_contract_violation(std::nothrow, "manual");
}

int main() {
  std::signal(SIGABRT, on_abort);
  g();
  return 1; // a [[noreturn]] function returned
}

// REQUIRES: contracts-libcxx, native
