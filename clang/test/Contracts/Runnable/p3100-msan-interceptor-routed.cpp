// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// RUN: env MSAN_OPTIONS=halt_on_error=1 %t 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_enforce \
// RUN:   %libcxx_flags -o %t.enforce && not %t.enforce 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ENFORCE

// An uninitialized-memory use detected by an MSan interceptor (here write) is
// routed like one detected by instrumented code: the handler owns all output
// (no stock "Uninitialized bytes in ..." line), observe continues whatever
// halt_on_error says, and enforce terminates after the handler.
//
// No GCC mirror: GCC has no MSan.

#include <contracts>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::fprintf(stderr, "handler\n");
}

int main() {
  int fd = open("/dev/null", O_WRONLY);
  char buf[4];
  (void)!write(fd, buf, sizeof buf); // interceptor-detected UMR
  std::fprintf(stderr, "survived\n");
  return 0;
}

// CHECK-NOT: Uninitialized bytes
// CHECK: handler
// CHECK: survived

// ENFORCE-NOT: Uninitialized bytes
// ENFORCE: handler
// ENFORCE-NOT: survived
