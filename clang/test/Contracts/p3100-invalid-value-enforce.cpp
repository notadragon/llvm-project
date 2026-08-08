// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-invalid-value-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: invalid bool value load, enforce -- handler runs, then terminates.

#include <contracts>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation&) {}

__attribute__((noinline)) bool load(const bool* p) { return *p; }

int main() { unsigned char c = 4; bool b; __builtin_memcpy(&b, &c, 1); return load(&b); }
