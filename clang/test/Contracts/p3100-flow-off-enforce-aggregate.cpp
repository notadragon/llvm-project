// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: flow-off-end under enforce catches the bug (handler, then terminate)
// regardless of return type -- here an aggregate / sret return.

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) {}

struct Big { int a[8]; };
Big f (int x) { if (x > 0) return Big{}; }   // falls off for x <= 0

int main () { Big b = f (-1); (void) b; return 0; }
