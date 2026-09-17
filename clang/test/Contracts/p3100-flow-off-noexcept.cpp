// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-noexcept.json %libcxx_flags -o %t && %t

// P3100 x D4298: flow-off-end ({stmt.return.flow.off}) configured to the
// non-throwing "noexcept_observe" semantic.  The handler runs and execution
// continues, returning a defined (erroneous) 0.

#include <contracts>
#include <cstdlib>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int classify(int x) {
  if (x > 0)
    return x * 2;
  // x <= 0: control flows off the end.
}

int main() {
  int r = classify(-1);
  if (violations != 1) std::abort();
  if (r != 0) std::abort();
}
