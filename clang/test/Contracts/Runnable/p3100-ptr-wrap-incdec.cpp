// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: the pointer-wrap check (ub:expr.add.out.of.bounds.pointer.arithmetic)
// covers every pointer increment and decrement: ++p and p++ past the top,
// --p and p-- below zero are caught and leave the pointer unmoved; in-range
// ones, and an in-range read through a subscript of a pointer, are not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-incdec.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
#include <cstdint>
int main() {
  char *top = reinterpret_cast<char *>(UINTPTR_MAX);
  char *low = nullptr;
  long r = 0;
  char *p = top; ++p; r += p - top;      // caught -> 0
  p = top; p++; r += p - top;            // caught -> 0
  p = low + 0; --p; r += p - low;        // caught -> 0
  p = low + 0; p--; r += p - low;        // caught -> 0
  char buf[4] = {1, 2, 3, 4};
  p = buf; ++p; r += p - buf;            // 1: fine
  p = buf + 2; --p; r += p - buf;        // 1: fine
  char *q = buf + 1;
  volatile long vi = -2;
  r += q[vi + 1];                        // 1 = buf[0]: fine
  std::printf("RESULT r=%ld viols=%d\n", r, viols);
  return 0;
}
// CHECK: RESULT r=3 viols=4
