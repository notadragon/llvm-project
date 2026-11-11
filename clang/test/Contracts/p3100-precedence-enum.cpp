// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=enum -fsanitize-semantic=enum:noexcept_observe \
// RUN:   -fcontract-configuration-file=%S/p3100-precedence.json \
// RUN:   %libcxx_flags -o %t && %t

// Where a P3100 implicit check and the sanitizer check are handled at the same
// place, the contract configuration wins; the sanitizer checks only what no
// contract covers.  Here for an invalid enum load: the configuration selects
// noexcept_enforce, the sanitizer is routed at noexcept_observe, and the
// handler exits 0 only when it sees the configuration's semantic.
//
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-precedence-enum.C

#include <contracts>
#include <cstdlib>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  // Exit 0 only for the configuration's semantic.
  std::_Exit(v.semantic() ==
                     std::contracts::evaluation_semantic::noexcept_enforce
                 ? 0
                 : 1);
}

enum E { e0, e1, e2 };
__attribute__((noinline)) int op(E *p) { return *p; }

int main() {
  volatile int r = 0;
  int x = 100;
  E e;
  std::memcpy(&e, &x, sizeof e);
  r = op(&e);
  return 2; // no handler call at all
}

// REQUIRES: contracts-libcxx, native
