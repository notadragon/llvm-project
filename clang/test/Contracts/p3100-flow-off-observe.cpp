// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-observe.json %libcxx_flags -o %t && %t

// P3100: the implicit contract assertion for falling off the end of a
// value-returning function ({stmt.return.flow.off}), configured via kind
// "implicit" to "observe": the handler runs, execution continues, a defined
// (erroneous) 0 is returned, and the violation reports assertion_kind::implicit.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-flow-off-config.C)

#include <contracts>
#include <cstdlib>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++violations;
  if (v.kind() != std::contracts::assertion_kind::implicit)
    std::abort();
}

int classify(int x) {
  if (x > 0)
    return x * 2;
  // x <= 0: control flows off the end.
}

int main() {
  int r = classify(-1);
  if (violations != 1) std::abort();  // handler ran exactly once
  if (r != 0) std::abort();           // observe continues, defined 0
}
