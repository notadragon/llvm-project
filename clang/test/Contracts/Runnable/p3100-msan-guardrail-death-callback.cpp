// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s

// Under contract routing the runtime guardrail refuses
// __sanitizer_set_death_callback, whichever sanitizer's checks are routed: a
// callback would interpose on the terminate path that contract enforcement
// drives.  Here MSan checks are routed to the contract-violation handler, and
// the setter dies with the guardrail message.
//
// No GCC mirror: GCC has no MSan.

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
