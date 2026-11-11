// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a read through `p->*pm` is followed by a load, so with only the
// .access leaf of ub:expr.unary.dereference.nullptr configured its null
// check is made, and made on `p` itself: a null `p` and a member pointer to a
// member at a nonzero offset, whose loaded address `p + pm` is not null, are
// caught.  (The handler ends the program: execution cannot continue past a
// null dereference.)  p3100-null-split.cpp pins the leaf in the IR.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-null-split-mptr-offset.C, and
// p3100-null-split-mptr-access.C and -indirection.C for the leaves.)

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
  int n;
};
__attribute__((noinline)) int read(S *p, int S::*pm) { return p->*pm; }
int main() {
  S s = {1, 2};
  int r = read(&s, &S::n);
  std::printf("clean r=%d\n", r);
  std::fflush(stdout);
  r = read(nullptr, &S::n);
  std::printf("missed r=%d\n", r);
  return 0;
}
// CHECK: clean r=2
// CHECK-NEXT: VIOL [null pointer dereference]
// CHECK-NOT: missed
