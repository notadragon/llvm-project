// P3100 RF5 (CL2): the ASan report populator is LAZY -- it runs ONLY if the
// handler calls contract_violation::report().  Here the handler does NOT call
// report(), so the populator never runs and the sanitizer prints nothing.  The
// only output is the handler's own "HANDLED" line; no ASan report text (no
// "heap-buffer-overflow", no "#0 " frame, no "=====" banner) appears anywhere.
// Under the default -fcontracts-p4298 -fsanitize=address the routed check
// resolves to noexcept_enforce, so the program terminates after the handler.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t > %t.out 2>&1; FileCheck %s < %t.out

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler that deliberately does NOT call v.report().  The
// populator must therefore never run, and no ASan report text must appear.
void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("HANDLED\n");
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  int *p = (int *)std::malloc(4 * sizeof(int));
  sink = oob(p, 100);
  std::free(p);
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// The handler ran, but report() was never called, so the populator never ran:
// the ONLY output is "HANDLED".  No ASan report text appears anywhere (proving
// laziness); the enforce termination is asserted by `not`.
// CHECK: HANDLED
// CHECK-NOT: heap-buffer-overflow
// CHECK-NOT: AddressSanitizer
// CHECK-NOT: {{^#0}}
// CHECK-NOT: ====
// CHECK-NOT: survived
