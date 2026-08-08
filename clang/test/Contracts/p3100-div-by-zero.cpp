// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero.json %libcxx_flags -o %t && %t

// P3100: integer division / remainder by zero ({expr.mul.div.by.zero}).
// ignore -> a defined (erroneous) 0 without executing the trapping division;
// observe -> handler runs (assertion_kind::implicit), then continue with 0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-div-by-zero.C)

#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  int divi(int a, int b) { return a / b; }
  int modi(int a, int b) { return a % b; }
}
namespace obs_ns {                    // observe
  int divi(int a, int b) { return a / b; }
  int modi(int a, int b) { return a % b; }
}

int main() {
  if (ign_ns::divi(10, 0) != 0) std::abort();
  if (ign_ns::modi(10, 0) != 0) std::abort();
  if (calls != 0) std::abort();

  if (obs_ns::divi(10, 0) != 0) std::abort();
  if (obs_ns::modi(10, 0) != 0) std::abort();
  if (calls != 2) std::abort();
  if (!all_implicit) std::abort();

  if (ign_ns::divi(10, 2) != 5) std::abort();
  if (obs_ns::modi(10, 3) != 1) std::abort();
  if (calls != 2) std::abort();
}
