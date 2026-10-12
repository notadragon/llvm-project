// P3100 parity fix: the runtime guardrail in
// __asan_set_error_report_callback must fire under quick_enforce routing too,
// not just noexcept_observe/noexcept_enforce.  quick_enforce terminates
// WITHOUT ever calling the contract-violation handler and WITHOUT printing
// anything on the error path (see p3100-asan-route-quick.cpp), so a program
// registering a stock callback under quick_enforce would otherwise see no
// visible conflict at all -- the callback would simply never fire, silently.
// The guardrail must still refuse the registration up front, at the setter
// call, which is what makes this observable even though quick_enforce would
// otherwise Die() silently on the actual sanitizer error.
//
// address:quick_enforce does not require -fcontracts-p4298 (see
// p3100-sanitize-semantic.cpp), so it is requested explicitly here rather
// than relying on it being the "no -fcontracts-p4298" default.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:quick_enforce \
// RUN:   %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <cstdio>

extern "C" void __asan_set_error_report_callback(void (*)(const char *));

static void my_cb(const char *) {}

int main() {
  // Routing is active (quick_enforce), so this call must not return: it
  // reports and Die()s at the setter, before any ASan error is ever detected.
  __asan_set_error_report_callback(my_cb);
  std::printf("registered\n");
  return 0;
}

// The guardrail fires with a message naming the opt-out; "registered" is
// never printed.  Without the parity fix (adding kAsanContractQuick to the
// guard condition in asan_report.cpp) this test fails: the setter would
// silently succeed, "registered" would print, and the process would exit 0,
// so `not %t` would fail and the CHECK lines below would not match.
// CHECK: stock error-report callbacks are disabled under contract routing
// CHECK-SAME: -fcontracts-p3100
// CHECK-SAME: -fsanitize-noncontract-callbacks
// CHECK-NOT: registered
