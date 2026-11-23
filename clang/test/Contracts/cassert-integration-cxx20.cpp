// RUN: %clangxx -std=c++20 %s -fcontracts-p3290 %libcxx_flags -o %t && %t

// Under -fcontracts-p3290 libc++ integrates assert from C++20, where it also
// defines __cpp_lib_assert_can_use_contracts: a failed assert reaches the
// contract-violation handler as assertion_kind::cassert.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/cassert-integration-cxx20.C

#include <version>
#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>
#include <contracts>
#include <cstdlib>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined"
#endif

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.kind() == std::contracts::assertion_kind::cassert)
    std::_Exit(0);
}

int main() {
  int zero = 0;
  assert(zero == 1);
  return 1;
}

// REQUIRES: contracts-libcxx, native
