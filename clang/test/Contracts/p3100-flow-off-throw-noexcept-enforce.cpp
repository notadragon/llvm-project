// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: a THROWING handler at flow-off-end in a NOEXCEPT function (enforce):
// the exception reaches the noexcept boundary and calls std::terminate.

#include <contracts>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation&) {
  throw E{};
}

int f(int x) noexcept { if (x > 0) return x; }   // enforce; falls off for x<=0

int main() { f(-1); }
