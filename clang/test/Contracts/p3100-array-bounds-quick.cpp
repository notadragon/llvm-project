// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-array-bounds-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: array subscript out of bounds, quick_enforce -- traps.

#include <cstring>
int g[4] = { 1, 2, 3, 4 };
__attribute__((noinline)) int rd(int i) { return g[i]; }
int main() { return rd(100); }
