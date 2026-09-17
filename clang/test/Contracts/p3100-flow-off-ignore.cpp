// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-ignore.json %libcxx_flags -o %t && %t

// P3100: flow-off-end ({stmt.return.flow.off}) configured to "ignore": no
// handler is invoked and the function returns a defined (erroneous) value --
// zero for a scalar return type -- rather than leaving undefined behavior.

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
  if (violations != 0) std::abort();  // ignore never calls the handler
  if (r != 0) std::abort();           // defined fallback value
}
