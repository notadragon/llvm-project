// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-float-cast-location.json %libcxx_flags -o %t && %t
// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-float-cast-location.json %libcxx_flags -o %t.O2 && %t.O2
// REQUIRES: contracts-libcxx

// P3100: the implicit float-to-integer conversion semantic
// (ub:conv.fpint.float.not.represented) is resolved per site and honors
// source line ranges in the configuration.  Two conversions in the same
// namespace on different lines resolve differently, so only the source
// location can distinguish them.
//
// Config (p3100-float-cast-location.json), first match wins (it names the
// ignored line by number, so keep this header's length):
//   1. the line of cvt_ignored -> ignore  (defined 0, no handler)
//   2. default                 -> observe (handler runs, then defined 0)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3100-float-cast-location.C

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation(const cs::contract_violation &) { ++calls; }

namespace same_ns {
int cvt_ignored(double d) { return (int)d; }  // ignored line
int cvt_observed(double d) { return (int)d; }
} // namespace same_ns

int main() {
  // Line-matched: ignore, defined 0, no handler call.
  if (same_ns::cvt_ignored(1e300) != 0) __builtin_abort();
  if (calls != 0) __builtin_abort();

  // Out of range: observe, the handler runs, then defined 0.
  if (same_ns::cvt_observed(1e300) != 0) __builtin_abort();
  if (calls != 1) __builtin_abort();
}
