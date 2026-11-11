// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-enforce.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

// P3100: flow-off-end under enforce catches the bug (handler, then terminate)
// regardless of return type -- here an aggregate / sret return.

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation&) {
  std::fputs("contract handler ran\n", stderr);}

struct Big { int a[8]; };
Big f (int x) { if (x > 0) return Big{}; }   // falls off for x <= 0

int main () { Big b = f (-1); (void) b; return 0; }

// The handler runs before termination; a native trap would not print this.
// CHECK: contract handler ran

// REQUIRES: contracts-libcxx, native
