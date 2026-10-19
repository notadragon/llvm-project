// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:observe \
// RUN:   %libcxx_flags -o %t.access
// RUN: %t.access star_read 2>&1 | FileCheck %s --check-prefix=CAUGHT
// RUN: %t.access sub_read 2>&1 | FileCheck %s --check-prefix=CAUGHT
// RUN: %t.access star_addr 2>&1 | FileCheck %s --check-prefix=OTHER
// RUN: %t.access sub_addr 2>&1 | FileCheck %s --check-prefix=OTHER
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:observe \
// RUN:   %libcxx_flags -o %t.indir
// RUN: %t.indir star_addr 2>&1 | FileCheck %s --check-prefix=CAUGHT
// RUN: %t.indir sub_addr 2>&1 | FileCheck %s --check-prefix=CAUGHT

// P3100: the object expression of a `.*` access is checked for null like
// that of a class member access: (*q).*pm and q[i].*pm check `q` itself --
// under the .access leaf of ub:expr.unary.dereference.nullptr where the
// member is read, under .indirection where only its address is formed -- so
// a null `q` and a member at a nonzero offset, whose address q + pm is not
// null, are caught.  (Regression test for CLANG-743, where only the
// member's address was checked.  The handler ends the program: execution
// cannot continue past a null dereference.)  p3100-null-split-mptr.cpp pins
// the leaves in the IR.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-null-split-dot-mptr-access.C
// and p3100-null-split-dot-mptr-indirection.C.)

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
__attribute__((noinline)) int star_read(S *q, int S::*pm) { return (*q).*pm; }
__attribute__((noinline)) int sub_read(S *q, int i, int S::*pm) {
  return q[i].*pm;
}
__attribute__((noinline)) int *star_addr(S *q, int S::*pm) {
  return &((*q).*pm);
}
__attribute__((noinline)) int *sub_addr(S *q, int i, int S::*pm) {
  return &(q[i].*pm);
}
int main(int argc, char **argv) {
  const char *c = argc > 1 ? argv[1] : "";
  int r = 0;
  void *a = nullptr;
  if (!std::strcmp(c, "star_read"))
    r = star_read(nullptr, &S::n);
  if (!std::strcmp(c, "sub_read"))
    r = sub_read(nullptr, 0, &S::n);
  if (!std::strcmp(c, "star_addr"))
    a = star_addr(nullptr, &S::n);
  if (!std::strcmp(c, "sub_addr"))
    a = sub_addr(nullptr, 0, &S::n);
  std::printf("missed %d %p\n", r, a);
  return 0;
}
// CAUGHT: VIOL [null pointer dereference]
// CAUGHT-NOT: missed
// The other leaf alone does not report it.
// OTHER-NOT: VIOL
// OTHER: missed
