// P3100 Task 3.1 + 3.2: the -fsanitize-noncontract-callbacks opt-out
// disengages BOTH the report routing and the runtime guardrail.  With the
// opt-out, no routing descriptor is emitted, so __asan_set_error_report_callback
// registers the stock callback normally (no abort at the call) and stock ASan
// reporting runs through it.  This is the same program as
// p3100-asan-guardrail-report-callback.cpp but built with the opt-out; there
// the call aborts, here it succeeds.

// No -fcontracts-p4298 here: with the opt-out set, routing is off, so the
// routed-semantic p4298 gate (SanitizerArgs::applyRoutedSemanticP4298Gate) is
// skipped -- exactly as GCC skips it under -fsanitize-noncontract-callbacks
// (gcc/opts.cc).  -fsanitize-recover=address without -fcontracts-p4298 would
// otherwise be a hard error under -fcontracts-p3100; the opt-out makes it
// compile CLEAN, exercising the real opt-out path.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-recover=address \
// RUN:   -fsanitize-noncontract-callbacks %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <cstdio>
#include <cstdlib>

extern "C" void __asan_set_error_report_callback(void (*)(const char *));

static void my_cb(const char *) {
  // Stock callback path: reached when ASan produces its report.
  std::printf("stock callback ran\n");
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  // With the opt-out, the guardrail is disengaged: this registers normally.
  __asan_set_error_report_callback(my_cb);
  std::printf("registered\n");
  std::fflush(stdout);

  int *p = (int *)std::malloc(4 * sizeof(int));
  sink = oob(p, 100);
  std::free(p);
  return 0;
}

// The callback registered without aborting (guardrail disengaged).
// CHECK: registered
// Stock ASan reporting still runs (no contract routing) ...
// CHECK: ERROR: AddressSanitizer: heap-buffer-overflow
// ... and drives the stock callback we installed.
// CHECK: stock callback ran
