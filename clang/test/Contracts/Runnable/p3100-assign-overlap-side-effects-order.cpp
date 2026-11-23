// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: the overlap check (ub:expr.assign.overlap) under "observe" keeps the
// meaning of an assignment whose operands have side effects: the right
// operand is evaluated before the left, so `a[k++] = a[k]` copies the element
// k names before the increment; each call in an operand is made once, and the
// right operand's first; the value read and the object stored to are the ones
// the operands designate, and the assignment's value is the stored one.  None
// of these overlap.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-side-effects-order.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
static int seq;
static int order[2];
int a[4] = {10, 20, 30, 40};
__attribute__((noinline)) int *left(int i) {
  order[0] = ++seq;
  return &a[i];
}
__attribute__((noinline)) int *right(int i) {
  order[1] = ++seq;
  return &a[i];
}
int main() {
  int k = 1;
  a[k++] = a[k];                     // a[1] = a[1]: k read before the increment
  int r1 = a[1] * 100 + k;           // 2002
  int v = (*left(0) = *right(3));    // a[0] = a[3]
  std::printf("RESULT r1=%d a0=%d v=%d right=%d left=%d viols=%d\n", r1, a[0],
              v, order[1], order[0], viols);
  return 0;
}
// CHECK: RESULT r1=2002 a0=40 v=40 right=1 left=2 viols=0
