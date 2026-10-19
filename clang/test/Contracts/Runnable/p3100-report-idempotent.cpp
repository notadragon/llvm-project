// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=address %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

// contract_violation::report() is idempotent -- calling it more than once
// returns the same rendered diagnostic (the lazy populator result is cached, not
// regenerated).  White-boxed on the ASan-routed P3100 path, which installs a
// real lazy report populator.  Under the default noexcept_enforce the program
// terminates after the handler.
// (GCC mirror: g++.dg/asan/p3100-report-idempotent.C.)

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  const char *r1 = v.report();
  const char *r2 = v.report();
  bool same_content = r1 && r2 && std::strcmp(r1, r2) == 0;
  std::printf(same_content ? "REPORT-IDEMPOTENT\n" : "REPORT-DIFFERS\n");
  std::printf(r1 == r2 ? "SAME-PTR\n" : "DIFF-PTR\n");
  std::fflush(stdout);
}

volatile int sink;

int __attribute__((noinline)) oob(int *p, int i) { return p[i]; }

int main() {
  int *p = (int *)std::malloc(4 * sizeof(int));
  sink = oob(p, 100); // heap out-of-bounds read -> routed to the handler
  std::free(p);
  return 0;
}

// CHECK: REPORT-IDEMPOTENT
// CHECK-NEXT: SAME-PTR
