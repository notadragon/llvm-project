// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: modulo-by-zero parity with divide -- quick_enforce traps.

#include <contracts>

int m (int a, int b) { return a % b; }

int main () { return m (10, 0); }
