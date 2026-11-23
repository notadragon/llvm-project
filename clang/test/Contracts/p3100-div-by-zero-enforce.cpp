// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-enforce.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

// P3100: integer divide-by-zero, enforce -- handler runs, then terminates.

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation&) {
  std::fputs("contract handler ran\n", stderr);}

int divi(int a, int b) { return a / b; }

int main() { return divi(10, 0); }

// The handler runs before termination; a native trap would not print this.
// CHECK: contract handler ran

// REQUIRES: contracts-libcxx, native
