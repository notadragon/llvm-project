// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-observe.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

// P3100: divide-by-zero with a THROWING handler in a NOEXCEPT function -- the
// exception reaches the noexcept boundary and calls std::terminate.

#include <contracts>
#include <cstdio>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation&) {
  std::fputs("contract handler ran\n", stderr);
  throw E{};
}

int divi(int a, int b) noexcept { return a / b; }   // observe

int main() { return divi(1, 0); }

// The handler runs before termination; a native trap would not print this.
// CHECK: contract handler ran

// REQUIRES: contracts-libcxx, native
