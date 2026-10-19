// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wall -Wextra -Werror \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: pointer increments and decrements checked by the pointer-wrap check
// (ub:expr.add.out.of.bounds.pointer.arithmetic) under "observe" keep their
// meaning: a postfix form yields the old value and a prefix form the
// variable itself, an lvalue; the operand is evaluated once; a discarded
// postfix form draws no -Wunused-value warning; a pointer to an object of a
// larger type moves by its size; and a decrement from one element past
// another's end is fine.  None of them is reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-incdec-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
struct Big {
  double d[4];
};
int buf[4] = {1, 2, 3, 4};
int *ps[2] = {buf, buf + 2};
static int calls;
__attribute__((noinline)) int *&pick(int i) {
  ++calls;
  return ps[i];
}
int main() {
  long r = 0;
  int *p = buf;
  int *q = p++;
  r += (q - buf) * 10 + (p - buf);              // 1
  int **pp = &++p;
  r += *pp - buf;                               // 2
  p++;
  r += p - buf;                                 // 3
  int *e = buf + 4;
  r += *--e;                                    // 4
  r += *pick(1)++;                              // 3
  r += ++pick(0) - buf;                         // 1
  Big bigs[2];
  Big *bp = bigs;
  ++bp;
  r += (unsigned long)((char *)bp - (char *)bigs) == sizeof(Big); // 1
  std::printf("RESULT r=%ld calls=%d viols=%d\n", r, calls, viols);
  return 0;
}
// CHECK: RESULT r=15 calls=2 viols=0
