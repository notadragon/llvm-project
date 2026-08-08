// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-fntryblock-observe.json %libcxx_flags -o %t && %t

// P3100: a function-try-block on a NOEXCEPT function still catches the flow-off
// throw with its own handler -- the exception never reaches the noexcept
// boundary because the handler catches it first -- so the function returns
// normally from the handler.

#include <contracts>
#include <cstdlib>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation&) {
  throw E{};
}

int f(int x) noexcept try { if (x > 0) return x; } catch (E&) { return -1; }

int main() {
  if (f(-1) != -1) std::abort();   // handler caught the throw, returned
  if (f(5) != 5) std::abort();
}
