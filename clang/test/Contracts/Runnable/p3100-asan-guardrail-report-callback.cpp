// P3100: runtime guardrail.  When contract routing is active (the
// front end emitted the descriptor under -fcontracts-p3100 with the address
// check resolved to a routed semantic), calling the stock setter
// __asan_set_error_report_callback must abort with a fatal error naming the
// -fsanitize-noncontract-callbacks opt-out, rather than silently registering a
// stock callback that mixes with contract routing.  The guardrail fires the
// same way regardless of which routed semantic (noexcept_observe /
// noexcept_enforce / quick_enforce) is in effect; this test exercises
// noexcept_enforce (wire 2, via -fcontracts-p4298), quick_enforce (wire 3) is
// covered separately in p3100-asan-guardrail-report-callback-quick.cpp.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <cstdio>

extern "C" void __asan_set_error_report_callback(void (*)(const char *));

static void my_cb(const char *) {}

int main() {
  // Routing is active, so this call must not return: it reports and Die()s.
  __asan_set_error_report_callback(my_cb);
  std::printf("registered\n");
  return 0;
}

// The guardrail fires with a message naming the opt-out; "registered" is
// never printed.
// CHECK: stock error-report callbacks are disabled under contract routing
// CHECK-SAME: -fcontracts-p3100
// CHECK-SAME: -fsanitize-noncontract-callbacks
// CHECK-NOT: registered
