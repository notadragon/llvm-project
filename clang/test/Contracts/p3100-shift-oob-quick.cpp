// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -Wno-shift-count-overflow -fcontract-configuration-file=%S/p3100-shift-oob-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: shift-out-of-range quick_enforce -- traps, no handler.

#include <contracts>

int shl (int a, int b) { return a << b; }

int main () { return shl (1, 100); }
