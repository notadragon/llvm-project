// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-fpint.json %libcxx_flags -o %t && %t

// P3100: converting a floating-point value to an integer type whose truncated
// value is not representable in the destination is core-language UB
// ({conv.fpint}).  ignore -> defined 0 without the trapping conversion;
// observe -> handler (assertion_kind::implicit) then 0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-fpint.C)

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
  int fi(double x) { return (int) x; }
}
namespace obs_ns {                    // observe
  int fi(double x) { return (int) x; }
}

int main() {
  // ignore: out-of-range and NaN -> defined 0, no handler.
  if (ign_ns::fi(1e30) != 0) std::abort();
  if (ign_ns::fi(-1e30) != 0) std::abort();
  if (ign_ns::fi(__builtin_nan("")) != 0) std::abort();
  if (calls != 0) std::abort();

  // observe: handler runs, then continue with 0.
  if (obs_ns::fi(1e30) != 0) std::abort();
  if (calls != 1) std::abort();
  if (!all_implicit) std::abort();

  // In-range conversions are unaffected (truncation toward zero).
  if (ign_ns::fi(3.9) != 3) std::abort();
  if (obs_ns::fi(-3.9) != -3) std::abort();
  if (calls != 1) std::abort();
}
