// RUN: %clang -fcontracts -std=c++26 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t
// RUN: %t
// (Previously XFAIL for missing __cxa_contract_violation_*_observe_pf link
// symbols; now resolved: -fcontracts auto-links libcontracts.)

#include "contracts.h"
#include "contracts-runtime.h"
#include "my_assert.h"

using namespace std::contracts;



int count = 0;
bool counter(bool value) {
  ++count;
  return value;
}

void test(const int x) pre(counter(x)) post(counter(x)) { contract_assert(counter(x)); }

int main() {
  test(1);
  assert(count == 3);
}
