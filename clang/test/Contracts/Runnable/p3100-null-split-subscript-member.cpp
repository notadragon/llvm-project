// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:observe \
// RUN:   %libcxx_flags -o %t.access
// RUN: %t.access 0 2>&1 | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:observe \
// RUN:   %libcxx_flags -o %t.indir
// RUN: %t.indir 1 2>&1 | FileCheck %s --check-prefix=CAUGHT

// P3100: the address of a member of an element, &q[i].n, forms an address
// only, so the null check of `q` is the .indirection leaf of
// ub:expr.unary.dereference.nullptr, as for &q->n: with only .access
// configured it is not reported, and with only .indirection configured `q`
// itself is checked, so a null `q` is caught for any `i`.  (Regression test
// for CLANG-744, where `q` was checked under .access, and under
// .indirection only the element's address q + i.)  p3100-null-split.cpp
// pins the leaves in the IR.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-null-split-subscript-member-
// access.C and -indirection.C; GCC checks the element's address, so it
// misses i != 0 -- CLANG-745, gnu_gcc GCC-794.)

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
__attribute__((noinline)) int *addr(S *q, int i) { return &q[i].n; }
int main(int argc, char **argv) {
  int i = argc > 1 ? std::atoi(argv[1]) : 0;
  std::printf("none %p\n", (void *)addr(nullptr, i));
  return 0;
}
// NONE-NOT: VIOL
// NONE: none
// CAUGHT: VIOL [null pointer dereference]
// CAUGHT-NOT: none
