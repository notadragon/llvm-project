// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-observe.json %libcxx_flags -o %t && %t

// P3100: when a throwing flow-off-end handler unwinds, the destructors of
// in-scope automatic variables in the function being fallen off must run.
// observe semantic.

#include <contracts>
#include <cstdlib>

struct E {};
struct S { static int dtors; ~S() { ++dtors; } };
int S::dtors = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  throw E{};
}

int f(int x) { S s; if (x > 0) return x; }   // falls off for x <= 0

int main() {
  bool caught = false;
  try { f(-1); } catch (E&) { caught = true; }   // flow-off throws -> unwind s
  if (!caught) std::abort();
  if (S::dtors != 1) std::abort();

  S::dtors = 0;
  if (f(5) != 5) std::abort();
  if (S::dtors != 1) std::abort();
}
