// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-group.json %libcxx_flags -o %t && %t

// P3595 dynamic selection via the [[clang::contract_group]] fallback.  The
// contract carries NO P3400 label -- only a [[clang::contract_group("safety")]]
// attribute -- and the config entry is keyed on that group with a "dynamic"
// output.  precomputeDynamicTable has no group_names from a label, so it falls
// back to the contract_group attribute (mirroring resolveContractConfig) when
// building the query; this makes the dynamic scan match the same entry the
// scalar scan matched.  Without that fallback the query would carry no group,
// the entry would not match, isDynamic() would stay false, and the selector
// would never be consulted.
//
// Compile-time default is "ignore"; the runtime selector returns "observe", so
// the failing precondition is handled exactly once and execution continues.

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

evaluation_semantic p3595_group_sel() { return evaluation_semantic::observe; }

void f(const int x) pre [[clang::contract_group("safety")]] (x > 0) {}

int main() {
  f(-1);
  if (violations != 1)
    std::abort();
  f(1);
  if (violations != 1)
    std::abort();
}
