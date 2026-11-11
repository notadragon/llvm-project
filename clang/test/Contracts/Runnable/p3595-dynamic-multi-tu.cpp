// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-configuration-file=%t/config.json %libcxx_flags \
// RUN:   %t/main.cpp %t/sel.cpp -o %t/a.out
// RUN: %t/a.out

// P3595 dynamic: multi-TU weak-default vs strong-override linking.  main.cpp
// holds the contract (a weak selector definition returning the config default
// "ignore" is emitted alongside it); sel.cpp provides a strong definition of the
// same selector returning "observe".  At link the strong definition wins, so the
// failing precondition is observed (handler fires once) instead of ignored.
// All prior dynamic tests were single-TU.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-dynamic-multi-tu.C)

//--- config.json
[{"output": {"semantic": "ignore", "dynamic": {"linkage": "C++", "name": "p3595_mt_sel"}}}]

//--- main.cpp
#include <contracts>

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++fired;
}

void f(int x) pre(x > 0) { }

int main() {
  f(-1);                       // strong selector -> observe -> fire once
  if (fired != 1) __builtin_abort();
}

//--- sel.cpp
#include <contracts>

std::contracts::evaluation_semantic p3595_mt_sel() {
  return std::contracts::evaluation_semantic::observe;
}
