// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-fpint-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: float-to-integer conversion out of range, enforce -- handler runs,
// then terminates.

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation&) {}

int fi(double x) { return (int) x; }

int main() { return fi(1e30); }
