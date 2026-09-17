// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-enforce.json %libcxx_flags -o %t && not --crash %t

// P3100: flow-off-end ({stmt.return.flow.off}) configured to "enforce": the
// handler runs and then the program terminates (the enforce entry point is
// noreturn).

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation&) {}

int classify(int x) {
  if (x > 0)
    return x * 2;
  // x <= 0: control flows off the end.
}

int main() { classify(-1); }
