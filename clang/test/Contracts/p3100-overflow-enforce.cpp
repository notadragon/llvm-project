// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-overflow-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: signed integer overflow, enforce -- handler runs, then terminates.

#include <contracts>
#include <climits>

void handle_contract_violation(const std::contracts::contract_violation&) {}

int mul(int a, int b) { return a * b; }

int main() { return mul(INT_MAX, 2); }
