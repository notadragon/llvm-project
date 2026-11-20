// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-location-line-column.json %libcxx_flags -o %t && %t

// P3595: "location": "file:line:column" matches the contract at that line
// and column (P3595 allows a column "for the rare occasions where such
// precision is needed"); a different column does not match.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3595-location-line-column.C
//
//
//
//
//

#include <contracts>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::abort(); // the entry did not match
}

//
//
//
//
// Keep f on line 27, with "pre" at column 15; the .json names both.

int f(int x)  pre(x > 0) { return x; }

int main() { return f(-1) == -1 ? 0 : 1; }

// REQUIRES: contracts-libcxx, native
