// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=signed-integer-overflow \
// RUN:   -fsanitize-semantic=signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t
// RUN: echo "signed-integer-overflow:*" > %t.supp
// RUN: env UBSAN_OPTIONS=suppressions=%t.supp %t

// A UBSan check routed to the contract-violation handler calls the handler for
// every violation, not only the first at a source location: UBSan's
// per-location dedup (SourceLocation::acquire, ignoreReport) does not apply to
// a routed report, and neither does a suppressions file (second RUN).
//
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-ubsan-routed-dedup.C

#include <contracts>
#include <climits>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
}

__attribute__((noinline)) int add(int x) { return x + 1; }

int main() {
  for (int i = 0; i < 3; ++i)
    add(INT_MAX);
  if (calls != 3)
    __builtin_abort();
}
