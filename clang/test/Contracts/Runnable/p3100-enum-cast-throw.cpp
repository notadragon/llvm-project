// P3100: a THROWING violation handler at an out-of-range enum-cast under
// "enforce".  The cast site is in a non-noexcept function, so the exception
// propagates out and is caught by the caller.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-enum-cast-enforce.json \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic()); std::fflush(stdout);
  throw Boom{};
}
enum E { A, B, C };
__attribute__((noinline)) E cast_it(int v) { return static_cast<E>(v); }
int main() {
  try { E e = cast_it(5); std::printf("RESULT=%d\n", (int)e); }
  catch (Boom &) { std::printf("CAUGHT\n"); std::fflush(stdout); }
  std::printf("SURVIVED\n");
  return 0;
}
// CHECK: VIOL sem=3
// CHECK: CAUGHT
// CHECK: SURVIVED
