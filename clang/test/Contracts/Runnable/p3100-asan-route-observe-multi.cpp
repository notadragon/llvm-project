// P3100 Bug #3 (routed behavior depends ONLY on the configured semantic, not on
// ASAN_OPTIONS).  Before the fix, compiler-rt's ScopedInErrorReport consulted
// halt_on_error_ and the one-shot __sanitizer_acquire_crash_state() latch before
// the routing branches, so under halt_on_error=1 the SECOND distinct-site
// violation was dropped (the first consumed the latch); and SuppressErrorReport
// honored suppress_equal_pcs, so whether a same-site repeat re-fired depended on
// that flag.  After the fix the routed path ignores those env-driven gates:
// every distinct site is delivered and a same-site repeat is reported ONCE
// (deterministically).  So the handler runs exactly twice here -- once per
// distinct site -- and the program continues, regardless of ASAN_OPTIONS.  Run
// under a challenging setting (halt_on_error=1:suppress_equal_pcs=0) that the
// pre-fix runtime mishandled.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t \
// RUN:   && env ASAN_OPTIONS=halt_on_error=1:suppress_equal_pcs=0 %t 2>&1 \
// RUN:   | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int calls = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
  std::printf("handler %d\n", calls);
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main(int argc, char **) {
  int *p = (int *)std::malloc(3 * sizeof(int));
  int i = argc + 4;             // >= 5, heap OOB, not constant-foldable
  sink = oob(p, i);             // site 1 (oob's PC)
  sink = p[i + 1];              // site 2 (a distinct PC, inlined here)
  for (int k = 0; k < 3; ++k)   // same PC as site 1 -> reported once, not thrice
    sink = oob(p, i);
  std::free(p);
  std::printf("done calls=%d\n", calls);
  std::fflush(stdout);
  return 0;
}

// Both distinct sites reported; the same-site repeat added no further call.
// CHECK: done calls=2
