// P3100 (UBSan runtime routing): on the routed vptr path the sanitizer emits
// NOTHING itself.  The report leg captures the rendered UBSan text live and the
// handler renders it ON DEMAND via contract_violation::report().  This proves
// the full vptr diagnostic appears only between the handler's REPORT markers.
// Default -fsanitize=vptr is recoverable, so with -fcontracts-p4298 the routed
// semantic is noexcept_observe and the program continues after the handler.

// (report() requires -fcontracts-p4301, which exposes __cpp_lib_contracts_report.)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=vptr %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("REPORT-BEGIN\n");
  std::fflush(stdout);
  const char *r = v.report();
  std::printf("%s\n", r ? r : "(null)");
  std::fflush(stdout);
  std::printf("REPORT-END\n");
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

// CHECK: REPORT-BEGIN
// CHECK: does not point to an object of type
// CHECK: REPORT-END
// CHECK: survived
