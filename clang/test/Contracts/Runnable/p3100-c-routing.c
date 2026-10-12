// P3100 sanitizer routing in C: a routed ASan stack overflow under
// noexcept_observe calls the handler once and execution continues.
//
// Mirror: gcc/testsuite/gcc.dg/asan/p3100-c-routing.c in the gnu_gcc fork.

// RUN: %clang -std=c23 %s -fcontracts-p4299 -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:noexcept_observe %libcxx_flags -o %t \
// RUN:   && %t

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
