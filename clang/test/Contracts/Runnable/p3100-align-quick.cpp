// P3100: misaligned access configured to "quick_enforce": traps, no handler.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-align-quick.json \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
int __attribute__((noinline)) load(int *p) { return *p; }
int main() {
  alignas(int) char buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  int *p = reinterpret_cast<int *>(buf + 1); // misaligned
  std::printf("RESULT=%d\n", load(p));
  return 0;
}
// CHECK-NOT: VIOL
