// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -Wno-shift-count-overflow -fcontract-configuration-file=%S/p3100-shift-oob-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: shift-out-of-range enforce -- handler runs, then terminate.

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) {}

int shl (int a, int b) { return a << b; }

int main () { return shl (1, 100); }
