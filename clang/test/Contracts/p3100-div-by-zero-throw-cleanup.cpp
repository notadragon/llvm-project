// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-observe.json %libcxx_flags -o %t && %t

// P3100: when a throwing divide-by-zero handler unwinds, the destructors of
// in-scope automatic variables must run (the guard's handler call is a proper
// unwinding call).  observe semantic.

#include <contracts>
#include <cstdlib>

struct E {};
struct S { static int dtors; ~S() { ++dtors; } };
int S::dtors = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  throw E{};
}

int main() {
  int x = 1, y = 0;   // runtime divisor
  bool caught = false;
  try {
    S s;
    int i = x / y;    // observe handler throws -> unwind must destroy s
    (void)i;
  } catch (E&) {
    caught = true;
  }
  if (!caught) std::abort();
  if (S::dtors != 1) std::abort();
}
