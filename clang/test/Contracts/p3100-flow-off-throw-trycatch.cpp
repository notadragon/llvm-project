// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-flow-off-throw-observe.json %libcxx_flags -o %t && %t

// P3100: a THROWING handler at flow-off-end, with the offending call inside a
// try { } catch (...) { } block: the exception is caught there and normal
// execution resumes.  observe semantic, non-noexcept function.

#include <contracts>
#include <cstdlib>

struct E {};
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

int classify(int x) { if (x > 0) return x * 2; }   // observe; falls off for x<=0

int main() {
  int steps = 0;
  try {
    classify(-1);   // flow-off -> observe handler throws -> caught below
    steps += 100;   // not reached
  } catch (E&) {
    steps += 1;
  }
  steps += 10;      // execution resumes here
  if (steps != 11) std::abort();
  if (calls != 1) std::abort();
}
