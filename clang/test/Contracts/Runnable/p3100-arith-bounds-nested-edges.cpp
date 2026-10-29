// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: nested known-bound pointer arithmetic under "observe" for the bounds
// checks (ub:expr.add.out.of.bounds.known) and the past-the-end indirection
// check (ub:expr.unary.dereference.invalid.known): in a template
// instantiation, on a member array, through a pointer to a member array, on
// the rows of a two-dimensional array, and on a member array of an object
// returned by a call, which is evaluated once, arithmetic past the bound and
// an indirection at it are caught and continue at the first element; a
// subscript is checked as a subscript of the array, (p->arr + 1)[i] as
// p->arr[1 + i]; arithmetic through a cast to another element type is not
// known-bound arithmetic.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-nested-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() != cs::assertion_kind::implicit)
    return;
  ++viols;
  std::printf("VIOL [%s]\n", v.comment());
}
int a[4] = {10, 20, 30, 40};
struct S {
  int arr[4];
  int tail;
};
S s = {{1, 2, 3, 4}, 5};
int m[2][3];
static int calls;
__attribute__((noinline)) S *get() {
  ++calls;
  return &s;
}

template <int K> __attribute__((noinline)) int *t(int j) {
  return (a + K) + j;
}
__attribute__((noinline)) int *mem(int i, int j) { return (s.arr + i) + j; }
__attribute__((noinline)) int ptrsub(S *p, int i) { return (p->arr + 1)[i]; }
__attribute__((noinline)) int (*rows(int i, int j))[3] { return (m + i) + j; }
__attribute__((noinline)) int *call(int j) { return (get()->arr + 1) + j; }
__attribute__((noinline)) int calld(int j) { return *(get()->arr + 1 + j); }
__attribute__((noinline)) char *cast(int j) { return (char *)(a + 1) + j; }

int main() {
  long r = 0;
  r += t<2>(2) - a;           // 4
  r += mem(1, 3) - s.arr;     // 4
  r += ptrsub(&s, 2);         // 4
  r += rows(1, 1) - m;        // 2
  r += call(3) - s.arr;       // 4
  r += calld(2);              // 4
  r += cast(12) - (char *)a;  // 16: not known-bound
  std::printf("clean=%d r=%ld calls=%d\n", viols, r, calls);
  r = 0;
  calls = 0;
  r += t<2>(3) - a;           // -> a: 0
  r += mem(1, 4) - s.arr;     // -> s.arr: 0
  r += ptrsub(&s, 3);         // -> s.arr[0]: 1
  r += rows(1, 2) - m;        // -> m: 0
  r += call(4) - s.arr;       // -> s.arr: 0
  r += calld(3);              // -> s.arr[0]: 1
  std::printf("caught=%d r=%ld calls=%d\n", viols, r, calls);
  return 0;
}
// CHECK: clean=0 r=38 calls=2
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [array subscript out of bounds]
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [indirection past the end of an array]
// CHECK-NEXT: caught=6 r=2 calls=2
