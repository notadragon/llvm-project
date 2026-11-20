// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: an indirection past the end of an array of known bound
// (ub:expr.unary.dereference.invalid.known) whose result is only bound to a
// reference or has its address taken is still an indirection: &*(a + N), a
// reference bound to *(a + N) or to a[N], and a[N] passed to a reference
// parameter are caught and continue at index 0; the same shapes in range are
// not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-known-address.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
int a[3] = {10, 20, 30};
volatile int vi;
__attribute__((noinline)) long off(const int &r) { return &r - a; }
int main() {
  long r = 0;
  vi = 3; r += &*(a + vi) - a;                 // caught -> 0
  vi = 3; { int &ref = *(a + vi); r += &ref - a; } // caught -> 0
  vi = 3; { int &ref = a[vi]; r += &ref - a; } // caught -> 0
  vi = 3; r += off(a[vi]);                     // caught -> 0
  vi = 2; r += &*(a + vi) - a;                 // 2: fine
  vi = 2; { int &ref = a[vi]; r += &ref - a; } // 2: fine
  std::printf("RESULT r=%ld viols=%d\n", r, viols);
  return 0;
}
// CHECK: RESULT r=4 viols=4
