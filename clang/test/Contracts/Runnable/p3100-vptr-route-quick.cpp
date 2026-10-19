// P3100 (UBSan runtime routing): -fsanitize-semantic=vptr:quick_enforce
// terminates WITHOUT calling the handler and WITHOUT any output -- the routed
// runtime Die()s silently.  quick_enforce needs no -fcontracts-p4298 (no handler
// is involved) and is available even though native vptr has no trap mode.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:quick_enforce %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>

// If the handler were (wrongly) invoked for quick_enforce it would print this;
// the CHECK-NOT below proves it is not.
void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("handler ran\n");
  std::fflush(stdout);
}

struct S { S() : a(0) {} virtual int v() { return 0; } int a; };
struct T : S { T() : b(0) {} int b; };

int __attribute__((noinline)) access_b(T *p) { return p->b; }

int main() {
  S s;
  T *p = reinterpret_cast<T *>(&s);
  volatile int sink = access_b(p);
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// quick_enforce = silent terminate: no handler, no stock UBSan report, no
// "survived".
// CHECK-NOT: handler ran
// CHECK-NOT: runtime error
// CHECK-NOT: survived
