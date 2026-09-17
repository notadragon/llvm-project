// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-overflow-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: signed integer overflow, quick_enforce -- traps on overflow.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-overflow-quick.C)

#include <contracts>
#include <climits>

int add(int a, int b) { return a + b; }

int main() { return add(INT_MAX, 1); }
