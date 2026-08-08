// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-overflow-noexcept.json %libcxx_flags -o %t && %t

// P3100 x P4298: signed overflow configured to noexcept_observe.  The nothrow
// handler runs with a populated contract_violation (kind=implicit, the right
// semantic, comment), then execution continues with the defined wrapped result.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-overflow-noexcept.C)

#include <contracts>
#include <climits>
#include <cstring>
#include <cstdlib>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit) std::abort();
  if (v.semantic() != cs::evaluation_semantic::noexcept_observe) std::abort();
  if (std::strcmp(v.comment(), "signed integer overflow") != 0) std::abort();
}

int add(int a, int b) { return a + b; }

int main() {
  int r = add(INT_MAX, 1);   // handler runs (nothrow), then continues
  if (calls != 1) std::abort();
  if (r != INT_MIN) std::abort();   // continued with the wrapped result
}
