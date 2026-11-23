// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a read (p->*pa)[i] of an element of an array member is followed by
// a load, so the null check of `p` is the .access leaf of
// ub:expr.unary.dereference.nullptr, as for p->arr[i]: with only .access
// configured, a null `p` and a member array at a nonzero offset, whose
// element address is not null, are caught.  (Regression test for CLANG-741,
// where `p` was checked only under .indirection.  The handler ends the
// program: execution cannot continue past a null dereference.)
// p3100-null-split-mptr.cpp pins the leaf in the IR.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-split-mptr-access.C, o5.)

#include <contracts>
#include <cstdio>
#include <cstdlib>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() != cs::assertion_kind::implicit)
    return;
  std::printf("VIOL [%s]\n", v.comment());
  std::fflush(stdout);
  std::_Exit(0);
}
struct S {
  int m;
  int arr[4];
};
__attribute__((noinline)) int read(S *p, int (S::*pa)[4], int i) {
  return (p->*pa)[i];
}
int main() {
  S s = {1, {2, 3, 4, 5}};
  int r = read(&s, &S::arr, 1);
  std::printf("clean r=%d\n", r);
  std::fflush(stdout);
  r = read(nullptr, &S::arr, 1);
  std::printf("missed r=%d\n", r);
  return 0;
}
// CHECK: clean r=3
// CHECK-NEXT: VIOL [null pointer dereference]
// CHECK-NOT: missed
