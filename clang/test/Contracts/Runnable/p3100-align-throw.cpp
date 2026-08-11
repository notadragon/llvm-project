// P3100: a THROWING handler at a misaligned access under "enforce".  Clang emits
// the guard in a valid EH context, so the exception propagates out of the
// (non-noexcept) accessor and is caught by the caller.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-align-enforce.json \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic()); std::fflush(stdout);
  throw Boom{};
}
int __attribute__((noinline)) load(int *p) { return *p; }
int main() {
  alignas(int) char buf[8] = {1,2,3,4,5,6,7,8};
  int *p = reinterpret_cast<int *>(buf + 1);
  try { std::printf("RESULT=%d\n", load(p)); }
  catch (Boom &) { std::printf("CAUGHT\n"); std::fflush(stdout); }
  std::printf("SURVIVED\n");
  return 0;
}
// CHECK: VIOL sem=3
// CHECK: CAUGHT
// CHECK: SURVIVED
