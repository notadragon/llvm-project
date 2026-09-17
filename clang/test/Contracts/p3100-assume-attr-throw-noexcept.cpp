// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-throw-noexcept.json %libcxx_flags -o %t && not --crash %t

// P3100: a throwing [[assume]] violation handler under observe in a noexcept
// function -- the exception reaches the noexcept boundary and terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-throw-noexcept.C)

#include <contracts>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation &) {
  throw E{};
}

namespace obs_ns { int f(int x) noexcept { [[assume(x > 0)]]; return x; } }

int main() { return obs_ns::f(-1); }
