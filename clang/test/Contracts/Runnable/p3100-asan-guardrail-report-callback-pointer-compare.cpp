// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-allow-assume -fsanitize=address,pointer-compare \
// RUN:   -fsanitize-semantic=address:assume,pointer-compare:noexcept_observe \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s

// P3100: runtime guardrail with only pointer-compare routed.  With the
// address check at assume (no descriptor) and pointer-compare routed,
// __asan_set_error_report_callback must still refuse a stock callback, since
// pointer-compare reports go to the contract-violation handler.
// p3100-asan-guardrail-report-callback-quick.cpp is the address-routed case.
//
// Mirror: gcc/testsuite/g++.dg/asan/p3100-guardrail-report-callback-pointer-compare.C

extern "C" void __asan_set_error_report_callback(void (*)(const char *));

static void my_cb(const char *) {}

int main() {
  __asan_set_error_report_callback(my_cb);
  return 0;
}

// CHECK: stock error-report callbacks are disabled under contract routing
