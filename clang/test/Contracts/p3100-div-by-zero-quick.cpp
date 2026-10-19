// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: integer divide-by-zero, quick_enforce -- traps, no handler.

#include <contracts>
#include <csignal>
#include <cstdlib>

// quick_enforce never calls the handler: a call exits normally, which
// `not --crash` reports as a failure.
void handle_contract_violation(const std::contracts::contract_violation &) {
  std::_Exit(0);
}

int divi(int a, int b) { return a / b; }

int main() {
  // So does the native SIGFPE the unchecked operation would raise.
  std::signal(SIGFPE, [](int) { std::_Exit(0); });
  return divi(10, 0);
}

// REQUIRES: contracts-libcxx, native
