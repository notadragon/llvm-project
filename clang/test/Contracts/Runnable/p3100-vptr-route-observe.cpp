// P3100 (UBSan runtime routing): under -fcontracts-p3100 a UBSan
// runtime check's report is routed to the contract-violation handler.  vptr is
// the first routed UBSan check.  With the vptr check resolved to noexcept_observe
// (via -fsanitize-recover=vptr + -fcontracts-p4298) the handler runs (kind=7,
// semantic=6) and the program CONTINUES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr -fsanitize-recover=vptr %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

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
  // noexcept_observe = report + continue: we reach here after the violation.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
