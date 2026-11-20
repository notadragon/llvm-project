// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-noexcept-enforce.json %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

// P3100 x P4298: null-dereference configured to noexcept_enforce -- the handler
// runs (nothrow) and the noreturn entry point terminates; the dereference is
// never reached.  The handler verifies the populated data before returning.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-noexcept-enforce.C)

#include <contracts>
#include <cstdio>
#include <cstdlib>

namespace cs = std::contracts;

void handle_contract_violation(const cs::contract_violation& v) {
  std::fputs("contract handler ran\n", stderr);
  if (v.kind() != cs::assertion_kind::implicit)
    std::exit(3);
  if (v.semantic() != cs::evaluation_semantic::noexcept_enforce)
    std::exit(3);
  // returns -> the noreturn noexcept_enforce entry terminates the program
}

int load_it(int *p) { return *p; }

int main() {
  int *p = nullptr;
  return load_it(p);
}

// The handler runs before termination; a native trap would not print this.
// CHECK: contract handler ran

// REQUIRES: contracts-libcxx, native
