// RUN: %clang -std=c23 %s -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t 2>&1 | FileCheck --allow-empty %s
// RUN: %t

// DECISIONS.md E175-E177 (CLANG-885): -fcontracts-p3100 turns on C contracts
// (-fcontracts-p4299), so a program compiled without -fcontracts-p4299
// replaces the violation handler for routed sanitizer checks too
// (p3100-c-routing.c gives it).  Under
// -fsanitize-semantic=address:noexcept_observe the stack overflow below calls
// the violation handler once and execution continues.
//
// Mirror: gcc/testsuite/gcc.dg/asan/p3100-c-routing-implied-p4299.c

// CHECK-NOT: {{has no effect|cannot be combined|error:}}

#include <contracts.h>

static int handler_called;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  handler_called++;
}

volatile int sink;

__attribute__((noinline)) int oob(int i) {
  int a[3] = {1, 2, 3};
  return a[i]; // stack out-of-bounds read: i >= 5
}

int main(int argc, char **argv) {
  (void)argv;
  sink = oob(argc + 4);
  // noexcept_observe: the handler ran once and execution continued.
  if (handler_called != 1)
    __builtin_abort();
  return 0;
}
