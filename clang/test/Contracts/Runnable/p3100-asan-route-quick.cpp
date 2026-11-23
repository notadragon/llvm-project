// P3100: with -fcontracts-p3100 but WITHOUT
// -fcontracts-p4298, the default -fsanitize=address resolves the routed address
// check to quick_enforce: the program TERMINATES and the contract-violation
// handler is NEVER called.  The sanitizer emits NOTHING on the routed
// path, so quick_enforce is a SILENT fast terminate: no ASan report, no
// "ABORTING" line, and no handler output at all.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>
#include <cstdlib>

// This handler must NOT run under quick_enforce.  If it were called it would
// print "handler ..." -- the CHECK-NOT lines below assert it is not.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d\n", (int)v.kind());
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  int *p = (int *)std::malloc(4 * sizeof(int));
  sink = oob(p, 100);
  std::free(p);
  // quick_enforce = terminate: we must NOT reach here.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// quick_enforce is a SILENT fast terminate: no handler, no ASan report, no
// ABORTING banner, and never reaches "survived".  The non-zero exit is asserted
// by `not`; these CHECK-NOTs assert nothing is emitted.
// CHECK-NOT: handler
// CHECK-NOT: AddressSanitizer
// CHECK-NOT: ABORTING
// CHECK-NOT: survived
