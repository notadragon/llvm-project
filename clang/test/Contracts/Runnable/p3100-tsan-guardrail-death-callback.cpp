// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s

// Under contract routing the runtime guardrail refuses
// __sanitizer_set_death_callback, whichever sanitizer's checks are routed: a
// callback would interpose on the terminate path that contract enforcement
// drives.  Here TSan checks are routed to the contract-violation handler, and
// the setter dies with the guardrail message.
//
// Mirror: gcc/testsuite/g++.dg/tsan/p3100-tsan-guardrail-death-callback.C

#include <cstdio>

extern "C" void __sanitizer_set_death_callback(void (*)(void));

static void cb() {}

int main() {
  // Routing is active, so this call must not return: it reports and Die()s.
  __sanitizer_set_death_callback(cb);
  std::printf("callback installed\n");
  return 0;
}

// CHECK: stock death callbacks are disabled under contract routing
// CHECK-NOT: callback installed
