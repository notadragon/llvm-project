// P3100: an IN-range enum-cast is not a violation even under "enforce" -- 3 is a
// valid value of enum E (range [0,3]) although no enumerator equals it.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-enum-cast-enforce.json \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &) {
  std::printf("UNEXPECTED-VIOLATION\n"); std::fflush(stdout);
}
enum E { A, B, C };
__attribute__((noinline)) E cast_it(int v) { return static_cast<E>(v); }
int main() { E e = cast_it(3); std::printf("RESULT=%d\n", (int)e); return 0; }
// CHECK-NOT: UNEXPECTED-VIOLATION
// CHECK: RESULT=3
