// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-array-bounds-noexcept.json %libcxx_flags -o %t && %t

// P3100 x P4298: array subscript out of bounds configured to noexcept_observe.
// The nothrow handler runs with a populated contract_violation, then the access
// uses index 0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-array-bounds-noexcept.C)

#include <contracts>
#include <cstring>
#include <cstdlib>

namespace cs = std::contracts;
static int calls = 0;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit) std::abort();
  if (v.semantic() != cs::evaluation_semantic::noexcept_observe) std::abort();
  if (std::strcmp(v.comment(), "array subscript out of bounds") != 0) std::abort();
}

int g[4] = { 10, 20, 30, 40 };
__attribute__((noinline)) int rd(int i) { return g[i]; }

int main() {
  int r = rd(100);           // handler runs (nothrow), then index 0
  if (calls != 1) std::abort();
  if (r != 10) std::abort();   // g[0]
}
