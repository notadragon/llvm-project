// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-fntryblock-observe.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

// P3100: a NOEXCEPT function-try-block whose handler runs off its own end.  The
// try body falls off -> inside check throws -> caught by the handler -> handler
// falls off -> after-construct check throws -> the exception escapes the
// noexcept boundary -> std::terminate.

#include <contracts>
#include <cstdio>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation&) {
  std::fputs("contract handler ran\n", stderr);
  throw E{};
}

int f(int x) noexcept try { if (x > 0) return x; } catch (E&) { /* no return */ }

int main() { f(-1); }

// The handler runs before termination; a native trap would not print this.
// CHECK: contract handler ran

// REQUIRES: contracts-libcxx, native
