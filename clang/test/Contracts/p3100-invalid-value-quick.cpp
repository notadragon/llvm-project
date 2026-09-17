// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-invalid-value-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: invalid bool value load, quick_enforce -- traps.

#include <cstring>

__attribute__((noinline)) bool load(const bool* p) { return *p; }

int main() { unsigned char c = 4; bool b; __builtin_memcpy(&b, &c, 1); return load(&b); }
