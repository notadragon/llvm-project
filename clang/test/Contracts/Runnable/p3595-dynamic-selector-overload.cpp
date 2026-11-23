// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-selector-overload.json %libcxx_flags -o %t && %t

// P3595: a "C++"-linkage dynamic selector is the function with that name
// taking no arguments (D3595R1 sec. "Configuration Outputs").  Overloads of
// the name that take parameters are different functions and are never
// called: the weak default, which returns the entry's semantic (observe),
// stays in force, so the violation is reported even though every overload
// the program defines would say ignore.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-dynamic-selector-overload.C)

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

namespace sel {
evaluation_semantic pick(int) { return evaluation_semantic::ignore; }
evaluation_semantic pick(const char *) { return evaluation_semantic::ignore; }
} // namespace sel

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

void f(int x) pre(x > 0) {}

int main() {
  f(0);
  if (violations != 1)
    std::abort();
}
