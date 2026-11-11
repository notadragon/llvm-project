// P3100 x P4301: the ASan pointer-compare and pointer-subtract checks, routed
// to the contract-violation handler, register the same lazy report populator
// as the address checks: contract_violation::report() renders the full ASan
// invalid-pointer-pair report on demand, non-null and non-empty, and calling
// it again returns the same text.  A violation no sanitizer produced -- a
// contract_assert -- has a null report, not an empty one (P4301: nullptr when
// no report is available).  p3100-asan-report-ondemand.cpp covers an address
// check.
// (GCC mirror: g++.dg/asan/p3100-pointer-pair-report-ondemand.C)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fcontract-evaluation-semantic=observe \
// RUN:   -fsanitize=address -fsanitize=pointer-compare,pointer-subtract \
// RUN:   -fsanitize-recover=pointer-compare,pointer-subtract %libcxx_flags -o %t \
// RUN:   && env ASAN_OPTIONS=detect_invalid_pointer_pairs=2:halt_on_error=0 %t \
// RUN:   2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  const char *r = v.report();
  const char *again = v.report();
  if (!r)
    std::printf("kind=%d report=null\n", (int)v.kind());
  else
    std::printf("kind=%d report=%s same=%d\n", (int)v.kind(),
                !*r                                     ? "empty"
                : std::strstr(r, "invalid-pointer-pair") ? "pair"
                                                         : "other",
                again && std::strcmp(r, again) == 0);
  std::fflush(stdout);
}

volatile long sink;

long __attribute__((noinline)) cmp(char *p, char *q) { return p < q; }

long __attribute__((noinline)) sub(char *p, char *q) { return p - q; }

int main() {
  char *a = (char *)std::malloc(42);
  char *b = (char *)std::malloc(42);
  sink = cmp(a, b);
  sink = sub(a, b);
  contract_assert(sink == 12345);
  std::free(a);
  std::free(b);
  std::printf("survived\n");
  return 0;
}

// CHECK-NOT: AddressSanitizer
// CHECK: kind=7 report=pair same=1
// CHECK-NEXT: kind=7 report=pair same=1
// CHECK-NEXT: kind=3 report=null
// CHECK-NEXT: survived
