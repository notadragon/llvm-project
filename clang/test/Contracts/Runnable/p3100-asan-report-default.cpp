// P4301 x P3100: with NO user handle_contract_violation, a routed ASan
// violation is delivered to the library's *default* handler, which calls
// report() itself and prints the full ASan diagnostic (heap-buffer-overflow, a
// "#0 " frame) ahead of its own basic contract-violation line, then terminates
// (noexcept_enforce under the default -fcontracts-p4298 -fsanitize=address).
// This is the default-handler end-to-end counterpart of
// p3100-asan-report-ondemand.cpp (which uses a user handler).
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s
// (GCC mirror: g++.dg/asan/p3100-report-default.C.)

#include <cstdlib>

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  int *p = (int *)std::malloc(4 * sizeof(int));
  sink = oob(p, 100);
  std::free(p);
  return 0;
}

// The default handler renders the routed ASan report on demand.
// CHECK: heap-buffer-overflow
// CHECK: #0
