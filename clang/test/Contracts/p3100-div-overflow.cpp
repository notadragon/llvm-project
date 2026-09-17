// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-overflow.json %libcxx_flags -o %t && %t

// P3100: signed division/remainder whose quotient is not representable
// (INT_MIN / -1, INT_MIN % -1) is core-language UB
// ({expr.mul.representable.type.result}).
// ignore -> defined 0 without executing the trapping op; observe -> handler
// (assertion_kind::implicit) then 0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-div-overflow.C)

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

constexpr int kMin = -__INT_MAX__ - 1;   // INT_MIN

namespace ign_ns {                    // ignore
  int divi(int a, int b) { return a / b; }
  int modi(int a, int b) { return a % b; }
}
namespace obs_ns {                    // observe
  int divi(int a, int b) { return a / b; }
  int modi(int a, int b) { return a % b; }
}

int main() {
  // ignore: INT_MIN / -1 and INT_MIN % -1 -> defined 0, no handler.
  if (ign_ns::divi(kMin, -1) != 0) std::abort();
  if (ign_ns::modi(kMin, -1) != 0) std::abort();
  if (calls != 0) std::abort();

  // observe: handler runs, continues with 0.
  if (obs_ns::divi(kMin, -1) != 0) std::abort();
  if (calls != 1) std::abort();
  if (!all_implicit) std::abort();

  // Non-overflowing signed division is unaffected (incl. -x / -1).
  if (ign_ns::divi(-10, -1) != 10) std::abort();
  if (obs_ns::divi(kMin, 2) != kMin / 2) std::abort();
  if (calls != 1) std::abort();
}
