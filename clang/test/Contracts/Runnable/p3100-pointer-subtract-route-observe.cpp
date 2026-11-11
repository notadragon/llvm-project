// P3100 (Clang): the ASan pointer-subtract check (subtracting two
// pointers into different objects -- expr.add.sub.diff.pointers UB) is routed to
// the contract-violation handler via its OWN wire byte
// (__asan_contract_semantic_pointer_subtract), independent of the address
// routing scope.  With -fsanitize-recover=pointer-subtract + -fcontracts-p4298
// the check resolves to noexcept_observe (kind=7, semantic=6) and the program
// CONTINUES.  detect_invalid_pointer_pairs must be enabled at run time.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize=pointer-subtract \
// RUN:   -fsanitize-recover=pointer-subtract %libcxx_flags -o %t \
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

volatile long sink;

long __attribute__((noinline)) sub(char *p, char *q) { return p - q; }

int main() {
  char *heap1 = (char *)std::malloc(42);
  char *heap2 = (char *)std::malloc(42);
  sink = sub(heap1, heap2);
  std::free(heap1);
  std::free(heap2);
  // observe = report + continue: we reach here after the violation.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
