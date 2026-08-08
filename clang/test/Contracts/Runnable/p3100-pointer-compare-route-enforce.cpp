// P3100 Task 4.1 (Clang): the ASan pointer-compare check ([expr.rel] UB) routed
// to the contract-violation handler.  With the default (no -fsanitize-recover=)
// + -fcontracts-p4298 the check resolves to noexcept_enforce: the handler runs
// (kind=7, semantic=7), then the program TERMINATES.  Governed by the
// pointer-compare wire byte, independent of the address routing scope.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize=pointer-compare %libcxx_flags -o %t \
// RUN:   && env ASAN_OPTIONS=detect_invalid_pointer_pairs=2:halt_on_error=1 \
// RUN:   not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) cmp(char *p, char *q) { return p < q; }

int main() {
  char *heap1 = (char *)std::malloc(42);
  char *heap2 = (char *)std::malloc(42);
  sink = cmp(heap1, heap2);
  std::free(heap1);
  std::free(heap2);
  // enforce = report + terminate: we must NOT reach here.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
