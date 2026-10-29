// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: arithmetic and subscripts applied to pointer arithmetic on an array
// of known bound N are checked against the same array, at the combined
// position, under "observe": (a + i) + j and (a + i) - j outside [0, N]
// (ub:expr.add.out.of.bounds.known.arithmetic), *(a + i + j) at N
// (ub:expr.unary.dereference.invalid.known), and (a + k)[i] as a[k + i] is
// (ub:expr.add.out.of.bounds.known.subscript for an access at or past N, or
// an address past N; .invalid.known for the address &(a + k)[N - k]),
// through three levels and with an unsigned index that would wrap the sum
// back into range.  Execution that continues past a violation uses a's first
// element.  Arithmetic within [0, N], and arithmetic on a pointer variable,
// are not reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-nested.C)

#include <contracts>
#include <cstddef>
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

__attribute__((noinline)) int *plus(int i, int j) { return (a + i) + j; }
__attribute__((noinline)) int *minus(int i, int j) { return (a + i) - j; }
__attribute__((noinline)) int *three(int i, int j, int k) {
  return a + i + j + k;
}
__attribute__((noinline)) int *uplus(int i, std::size_t j) {
  return (a + i) + j;
}
__attribute__((noinline)) int deref(int i, int j) { return *(a + i + j); }
__attribute__((noinline)) int sub(int i) { return (a + 1)[i]; }
__attribute__((noinline)) int *subaddr(int i) { return &(a + 1)[i]; }
__attribute__((noinline)) int *var(int i) {
  int *p = a + 1;
  return p + i;
}

int main() {
  int r = 0;
  r += plus(2, 2) - a;    // 4
  r += minus(3, 3) - a;   // 0
  r += three(1, 1, 2) - a; // 4
  r += uplus(3, 1) - a;   // 4
  r += deref(2, 1);       // 40
  r += sub(2);            // 40
  r += subaddr(2) - a;    // 3
  r += var(10) - a;       // 11: not a known-bound pointer
  std::printf("clean=%d r=%d\n", viols, r);
  r = 0;
  r += plus(2, 3) - a;    // -> a: 0
  r += minus(1, 2) - a;   // -> a: 0
  r += three(1, 2, 2) - a; // -> a: 0
  r += uplus(1, (std::size_t)-1) - a; // -> a: 0
  r += deref(2, 2);       // -> a[0]: 10
  r += sub(3);            // -> a[0]: 10
  r += subaddr(3) - a;    // &a[4]: -> a: 0
  r += subaddr(4) - a;    // -> a: 0
  std::printf("caught=%d r=%d\n", viols, r);
  return 0;
}
// CHECK: clean=0 r=106
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: VIOL [indirection past the end of an array]
// CHECK-NEXT: VIOL [array subscript out of bounds]
// CHECK-NEXT: VIOL [indirection past the end of an array]
// CHECK-NEXT: VIOL [array subscript out of bounds]
// CHECK-NEXT: caught=8 r=20
