// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-noexcept-observe.json %libcxx_flags -o %t && %t

// P3100 x P4298: null-dereference configured to noexcept_observe.  The handler
// runs (nothrow) with a correctly populated contract_violation (kind=implicit,
// the right semantic, the comment); here it verifies that data and exits, which
// also proves it was called (had it not been, the null load would crash).
// Without the exit, observe would proceed into the real dereference (crash).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-noexcept-observe.C)

#include <contracts>
#include <cstdlib>
#include <cstring>

namespace cs = std::contracts;

void handle_contract_violation(const cs::contract_violation& v) {
  if (v.kind() != cs::assertion_kind::implicit)
    std::abort();
  if (v.semantic() != cs::evaluation_semantic::noexcept_observe)
    std::abort();
  if (std::strcmp(v.comment(), "null pointer dereference") != 0)
    std::abort();
  std::exit(0);   // proves the handler ran with correct data
}

int load_it(int *p) { return *p; }

int main() {
  int *p = nullptr;
  load_it(p);
  std::abort();   // handler must have exited before we get here
}
