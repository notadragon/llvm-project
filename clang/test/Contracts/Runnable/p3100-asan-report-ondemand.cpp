// P3100: on the contract-routed ASan path the sanitizer emits NOTHING
// itself.  Instead it registers a lazy report populator (CXA_FIELD_REPORT) on
// the violation, and the handler renders the full ASan report ON DEMAND by
// calling contract_violation::report().  This test proves:
//   (a) nothing (no "=====" banner, no ASan text) appears BEFORE the handler,
//   (b) the full ASan report (heap-buffer-overflow, a "#0 " frame) appears only
//       BETWEEN the handler's REPORT-BEGIN and REPORT-END markers.
// Under the default -fcontracts-p4298 -fsanitize=address the routed check
// resolves to noexcept_enforce, so the program terminates after the handler.

// (report() requires -fcontracts-p4301, which exposes __cpp_lib_contracts_report.)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler.  Brackets the on-demand report with REPORT-BEGIN /
// REPORT-END so the checks below prove the ASan text is handler-owned (between
// the markers) and that nothing leaked before REPORT-BEGIN.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("REPORT-BEGIN\n");
  std::fflush(stdout);
  const char *r = v.report();
  std::printf("%s\n", r ? r : "(null)");
  std::fflush(stdout);
  std::printf("REPORT-END\n");
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

// Nothing before the handler: the FIRST line of output is REPORT-BEGIN (the
// "=====" banner and any pre-handler ASan line are suppressed on the routed
// path).  CHECK (not CHECK-NEXT after a leading CHECK-NOT) with the report text
// only between the markers proves the ordering; the enforce termination is
// asserted by `not`.
// CHECK-NOT: AddressSanitizer
// CHECK: REPORT-BEGIN
// CHECK: heap-buffer-overflow
// CHECK: #0
// CHECK: REPORT-END
