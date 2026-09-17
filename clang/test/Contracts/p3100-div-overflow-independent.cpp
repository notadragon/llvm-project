// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-overflow-independent.json %libcxx_flags -o %t && %t

// P3100: the two division UBs at a single site are configured independently --
// divide-by-zero as ignore, and signed-division-overflow as observe -- and each
// fires with its own semantic.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-div-overflow-independent.C)

#include <contracts>
#include <cstdlib>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
}

int divi(int a, int b) { return a / b; }

int main() {
  // divide-by-zero -> ignore: defined 0, no handler.
  if (divi(10, 0) != 0) std::abort();
  if (calls != 0) std::abort();

  // INT_MIN / -1 -> observe: handler runs, then 0.
  if (divi(-__INT_MAX__ - 1, -1) != 0) std::abort();
  if (calls != 1) std::abort();

  // ordinary division unaffected.
  if (divi(12, 3) != 4) std::abort();
  if (calls != 1) std::abort();
}
