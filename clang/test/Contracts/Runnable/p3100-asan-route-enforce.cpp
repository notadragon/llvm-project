// P3100 Task 2.2 (CL2): under -fcontracts-p3100, an ASan-detected error is
// routed to the contract-violation handler.  With the address check resolved to
// noexcept_enforce (the default under -fcontracts-p4298), the handler runs
// (reporting the violation as an implicit contract assertion, kind=7,
// semantic=7) and the program TERMINATES.  (ThinLTO descriptor survival is
// checked at the bitcode level in p3100-asan-descriptor.cpp, since this build
// ships no LTO-capable linker.)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  int *p = (int *)std::malloc(4 * sizeof(int));
  sink = oob(p, 100);
  std::free(p);
  // enforce = report + terminate: we must NOT reach here.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// The handler ran, reporting an implicit contract assertion with the
// noexcept_enforce ABI semantic.
// CHECK: handler kind=7 semantic=7
// enforce terminates: "survived" is never printed (the non-zero exit is
// asserted by `not`).
// CHECK-NOT: survived
