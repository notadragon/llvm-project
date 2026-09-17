// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-observe.json %libcxx_flags -o %t && %t

// P3100: divide-by-zero with a THROWING handler where the division sits in a
// function-try-block's try body -- the site is already inside the try scope, so
// the function's own handler catches the throw (no special handling needed).

#include <contracts>
#include <cstdlib>

struct E {};
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

int divi(int a, int b) try { return a / b; } catch (E&) { return -1; }

int main() {
  if (divi(1, 0) != -1) std::abort();   // caught by own handler
  if (calls != 1) std::abort();
  if (divi(10, 2) != 5) std::abort();
  if (calls != 1) std::abort();
}
