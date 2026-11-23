// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-location.json %libcxx_flags -o %t && %t
// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-location.json %libcxx_flags -o %t.O2 && %t.O2
// REQUIRES: contracts-libcxx

// P3100: the implicit flow-off-end semantic (ub:stmt.return.flow.off) is
// resolved per site and honors source line ranges in the configuration.  Two
// value-returning functions in the same namespace, each on one line, resolve
// differently, so only the source location can distinguish them.
//
// Config (p3100-flow-off-location.json), first match wins (it names the
// ignored line by number, so keep this header's length):
//   1. the line of f_ignored -> ignore  (zeroed return, no handler)
//   2. default               -> observe (handler runs, then zeroed return)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3100-flow-off-location.C

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation(const cs::contract_violation &) { ++calls; }

namespace same_ns {
int f_ignored(int x) { if (x) return x; }  // ignored line
int f_observed(int x) { if (x) return x; }
} // namespace same_ns

int main() {
  // Line-matched: ignore, zeroed return, no handler call.
  if (same_ns::f_ignored(0) != 0) __builtin_abort();
  if (calls != 0) __builtin_abort();

  // Out of range: observe, the handler runs, then zeroed return.
  if (same_ns::f_observed(0) != 0) __builtin_abort();
  if (calls != 1) __builtin_abort();
}
