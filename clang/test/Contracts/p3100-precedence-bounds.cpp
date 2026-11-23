// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=array-bounds -fsanitize-semantic=array-bounds:noexcept_observe \
// RUN:   -fcontract-configuration-file=%S/p3100-precedence.json \
// RUN:   %libcxx_flags -o %t && %t

// Where a P3100 implicit check and the sanitizer check are handled at the same
// place, the contract configuration wins; the sanitizer checks only what no
// contract covers.  Here for an out-of-bounds subscript: the configuration
// selects noexcept_enforce, the sanitizer is routed at noexcept_observe, and
// the handler exits 0 only when it sees the configuration's semantic.
//
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-precedence-bounds.C

#include <contracts>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  // Exit 0 only for the configuration's semantic.
  std::_Exit(v.semantic() ==
                     std::contracts::evaluation_semantic::noexcept_enforce
                 ? 0
                 : 1);
}

int g[4];
__attribute__((noinline)) int op(int i) { return g[i]; }

int main() {
  volatile int r = 0;
  r = op(6);
  return 2; // no handler call at all
}

// REQUIRES: contracts-libcxx, native
