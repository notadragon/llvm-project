// P3100 Task 4.1 (UBSan runtime routing): with the vptr check resolved to
// noexcept_enforce the handler runs (kind=7, semantic=7) and the program then
// TERMINATES.  The terminating semantic rides the sanitizer's NON-recovering
// (abort) code path.  (vptr is recover-by-default, so the enforce semantic must
// be requested explicitly -- default -fsanitize=vptr derives noexcept_observe.)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_enforce %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

struct S { S() : a(0) {} virtual int v() { return 0; } int a; };
struct T : S { T() : b(0) {} int b; };

int __attribute__((noinline)) access_b(T *p) {
  return p->b; // vptr: member access; *p is really an S, not a T.
}

int main() {
  S s;
  T *p = reinterpret_cast<T *>(&s);
  volatile int sink = access_b(p);
  (void)sink;
  std::printf("survived\n"); // must NOT be reached
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
