// P3100: under -fcontracts-p3100, resolving the address check to
// assume suppresses ASan instrumentation entirely -- byte-identical to a build
// without -fsanitize=address for that check.  No __asan_report* calls are
// emitted, so the out-of-bounds access is NOT detected and the program runs to
// completion ("survived").  (The IR-level "no instrumentation" check lives in
// p3100-asan-descriptor.cpp; this proves the runtime consequence.)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:assume %libcxx_flags \
// RUN:   -o %t && %t 2>&1 | FileCheck %s

#include <cstdio>
#include <cstdlib>

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  int *p = (int *)std::malloc(64 * sizeof(int));
  // assume = check off: not instrumented, so this in-heap (but logically
  // out-of-contract) read is not diagnosed; the program continues.
  sink = oob(p, 8);
  std::free(p);
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// assume suppresses instrumentation: no ASan diagnostic, program continues.
// CHECK-NOT: AddressSanitizer
// CHECK: survived
