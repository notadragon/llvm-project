// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

// The libc++ default violation handler gives no detection-mode phrase for a
// P3290 manual violation (detection mode unspecified: nothing was evaluated,
// nothing threw), rather than calling it a throwing predicate.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/default-handler-manual-mode.C

#include <contracts>

int main() {
  std::contracts::handle_observed_contract_violation("manual check");
  return 0;
}

// CHECK: manual check
// CHECK-NOT: exited via exception

// REQUIRES: contracts-libcxx, native
