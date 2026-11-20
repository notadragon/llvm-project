// P3100 full-coverage UBSan routing (Clang-only): local-bounds is trap-by-default,
// so routing clears its trap and drives the BoundsChecking pass to emit
// __ubsan_handle_local_out_of_bounds (which flows through libubsan's ScopedReport);
// noexcept_observe runs the handler (kind 7, semantic 6) and continues.  Needs -O1+
// (the pass relies on llvm.objectsize).

// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=local-bounds \
// RUN:   -fsanitize-semantic=local-bounds:noexcept_observe %libcxx_flags \
// RUN:   -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(), (int)v.semantic());
  std::fflush(stdout);
}

int __attribute__((noinline)) f() {
  int a[3] = {};
  volatile int i = 5;
  return a[i]; // out-of-bounds access on a local array of known size
}

int main() {
  volatile int r = f();
  (void)r;
  std::printf("survived\n");
  std::fflush(stdout);
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
