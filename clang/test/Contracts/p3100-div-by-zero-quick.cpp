// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: integer divide-by-zero, quick_enforce -- traps, no handler.

#include <contracts>

int divi(int a, int b) { return a / b; }

int main() { return divi(10, 0); }
