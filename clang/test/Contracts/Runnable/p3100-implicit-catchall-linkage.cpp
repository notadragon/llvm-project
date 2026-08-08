// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O \
// RUN:   -Wno-return-type \
// RUN:   -fcontract-configuration-file=%S/p3100-implicit-catchall-linkage.json \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P3100: a middle-end implicit check (signed integer overflow) routed to a
// handler semantic via a bare "kind": "implicit" catch-all configuration must
// link and run.  On GCC the shared contract descriptor table could be reclaimed
// with an inline <contracts> referrer, leaving the check's data block with a
// dangling descriptor -> link error; this exercises that end-to-end.  No Clang
// bug found (Clang's descriptor emission does not have the reclamation exposure;
// GCC F19/3866c160464 did not reproduce).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-catchall-linkage.C.)

#include <contracts>
#include <climits>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    __builtin_abort();
}

// Signed overflow -> middle-end check -> noexcept_observe (via the catch-all):
// handler runs, continues wrapped.
int add(int a, int b) { return a + b; }

int main() {
  if (add(INT_MAX, 1) != INT_MIN) __builtin_abort();   // overflow -> handler
  if (calls != 1) __builtin_abort();
  if (add(2, 3) != 5) __builtin_abort();               // no overflow, no call
  if (calls != 1) __builtin_abort();
}
