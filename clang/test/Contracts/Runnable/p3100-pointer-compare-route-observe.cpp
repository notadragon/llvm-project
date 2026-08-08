// P3100 Task 4.1 (Clang): the ASan pointer-compare check (comparing two pointers
// into different objects -- [expr.rel] UB) is routed to the contract-violation
// handler.  These pointer-pair checks report through the SAME ASan
// ScopedInErrorReport path as address errors, but via their OWN wire byte
// (__asan_contract_semantic_pointer_compare), so the address routing scope is
// unchanged.  With -fsanitize-recover=pointer-compare + -fcontracts-p4298 the
// check resolves to noexcept_observe: the handler runs (kind=7, semantic=6) and
// the program CONTINUES.  The check only fires when detect_invalid_pointer_pairs
// is set at run time.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize=pointer-compare \
// RUN:   -fsanitize-recover=pointer-compare %libcxx_flags -o %t \
// RUN:   && env ASAN_OPTIONS=detect_invalid_pointer_pairs=2:halt_on_error=0 %t \
// RUN:   2>&1 | FileCheck %s

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
  // observe = report + continue: we reach here after the violation.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
