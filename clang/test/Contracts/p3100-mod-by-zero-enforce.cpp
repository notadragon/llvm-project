// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-enforce.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

// P3100: modulo-by-zero parity with divide -- enforce terminates.

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation&) {
  std::fputs("contract handler ran\n", stderr);}

int m (int a, int b) { return a % b; }

int main () { return m (10, 0); }

// The handler runs before termination; a native trap would not print this.
// CHECK: contract handler ran

// REQUIRES: contracts-libcxx, native
