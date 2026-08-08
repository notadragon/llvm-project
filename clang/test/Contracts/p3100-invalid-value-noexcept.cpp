// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-invalid-value-noexcept.json %libcxx_flags -o %t && %t

// P3100 x P4298: invalid bool value load configured to noexcept_observe.  The
// nothrow handler runs with a populated contract_violation (kind=implicit, the
// right semantic, comment), then continues with the defined value.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-value-noexcept.C)

#include <contracts>
#include <cstring>
#include <cstdlib>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit) std::abort();
  if (v.semantic() != cs::evaluation_semantic::noexcept_observe) std::abort();
  if (std::strcmp(v.comment(), "invalid value for its type") != 0) std::abort();
}

__attribute__((noinline)) bool load(const bool* p) { return *p; }

int main() {
  unsigned char c = 4; bool b; std::memcpy(&b, &c, 1);
  bool r = load(&b);           // handler runs (nothrow), then continues
  if (calls != 1) std::abort();
  if (r != false) std::abort();   // defined value
}
