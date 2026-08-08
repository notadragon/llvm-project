// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-overflow-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: signed division overflow (INT_MIN / -1) enforce -- handler runs, then
// terminates.

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation&) {}

int divi(int a, int b) { return a / b; }

int main() { return divi(-__INT_MAX__ - 1, -1); }
