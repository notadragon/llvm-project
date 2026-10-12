// P3100: a properly aligned access is NOT a violation, even under enforce.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-align-enforce.json \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &) {
  std::printf("UNEXPECTED-VIOLATION\n"); std::fflush(stdout);
}
int __attribute__((noinline)) load(int *p) { return *p; }
int main() {
  alignas(int) char buf[8] = {1,2,3,4};
  int *p = reinterpret_cast<int *>(buf);  // aligned
  std::printf("RESULT=%d\n", load(p));
  return 0;
}
// CHECK-NOT: UNEXPECTED-VIOLATION
// CHECK: RESULT=
