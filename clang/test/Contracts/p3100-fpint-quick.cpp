// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-fpint-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: float-to-integer conversion out of range, quick_enforce -- traps, no
// handler.

#include <contracts>

int fi(double x) { return (int) x; }

int main() { return fi(1e30); }
