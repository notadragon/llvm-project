// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontract-configuration-file=%S/p3100-assume-nonpure-p4298.json \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P3100: an opaque (side-effecting / non-checkable) [[assume(cond)]] must never
// evaluate its predicate -- its allowed set is {assume, ignore}, so a configured
// checking semantic clamps to ignore (drop the assumption).  This must hold even
// under -fcontracts-p4298: the noexcept-variant widening of the allowed set must
// not re-admit a checking semantic the predicate's own allowed set excludes.
// No Clang bug found (GCC F18/d21082bae53 did not reproduce here).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-nonpure-p4298.C.)

#include <contracts>

static volatile int called = 0;
// Not pure/const: a call with side effects -> opaque predicate.
__attribute__((noinline)) bool sideeffect() { called = 1; return true; }

void handle_contract_violation(const std::contracts::contract_violation&) {}

__attribute__((noinline)) void g() { [[assume(sideeffect())]]; }

int main() {
  g();
  // Opaque predicate must be dropped, not evaluated, even under p4298.
  if (called != 0)
    __builtin_abort();
}
