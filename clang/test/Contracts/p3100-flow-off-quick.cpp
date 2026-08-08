// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: flow-off-end ({stmt.return.flow.off}) configured to "quick_enforce":
// the program terminates immediately (trap) without invoking the handler.

#include <contracts>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation&) {
  std::abort();  // quick_enforce must NOT call the handler
}

int classify(int x) {
  if (x > 0)
    return x * 2;
  // x <= 0: control flows off the end.
}

int main() { classify(-1); }
