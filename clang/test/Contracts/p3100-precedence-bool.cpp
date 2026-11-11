// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=bool -fsanitize-semantic=bool:noexcept_observe \
// RUN:   -fcontract-configuration-file=%S/p3100-precedence.json \
// RUN:   %libcxx_flags -o %t && %t

// Where a P3100 implicit check and the sanitizer check are handled at the same
// place, the contract configuration wins; the sanitizer checks only what no
// contract covers.  Here for an invalid bool load: the configuration selects
// noexcept_enforce, the sanitizer is routed at noexcept_observe, and the
// handler exits 0 only when it sees the configuration's semantic.
//
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-precedence-bool.C

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

__attribute__((noinline)) bool op(bool *p) { return *p; }

int main() {
  volatile int r = 0;
  unsigned char raw = 7;
  bool b;
  std::memcpy(&b, &raw, 1);
  r = op(&b);
  return 2; // no handler call at all
}

// REQUIRES: contracts-libcxx, native
