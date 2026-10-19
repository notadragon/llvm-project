// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-exceptions \
// RUN:   -fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a member referred to in a constructor function-try-block handler
// (ub:except.handle.handler.ctor.dtor) configured to "noexcept_enforce": the
// handler runs (kind=7, sem=7), then the program terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-handler-member-noexcept-enforce.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
static int *seen;
struct A {
  int x;
  int y = 42;
  A(int v) try : x(v == 1 ? throw 1 : 1) {
  } catch (int) {
    seen = &y; // y is referred to, not accessed
  }
};
int main() {
  try {
    A a(1);
  } catch (int) {
  }
  std::printf("RESULT=%d\n", seen != nullptr);
  return 0;
}
// CHECK: VIOL kind=7 sem=7 comment=[member referred to in a constructor or destructor function-try-block handler]
// CHECK-NOT: RESULT
