// P3100: out-of-range integer -> enumeration static_cast
// (ub:expr.static.cast.enum.outside.range) configured to "observe".  5 is
// outside enum E's value range [0,3] though it fits the underlying type.  The
// handler runs (sem=2); execution continues with the defined value 0.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-enum-cast-observe.json \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
enum E { A, B, C }; // [dcl.enum] value range [0,3]
__attribute__((noinline)) E cast_it(int v) { return static_cast<E>(v); }
int main() { E e = cast_it(5); std::printf("RESULT=%d\n", (int)e); return 0; }
// CHECK: VIOL kind=7 sem=2 comment=[enumeration value out of range]
// CHECK: RESULT=0
