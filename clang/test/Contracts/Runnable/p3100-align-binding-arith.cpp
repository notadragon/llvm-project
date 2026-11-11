// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:noexcept_observe \
// RUN:   %libcxx_flags -o %t.grp
// RUN: %t.grp | FileCheck %s

// P3100: the alignment check of a reference binding
// (ub:basic.align.object.alignment.store) reports a reference bound to
// `*(int *)(a + o)`, a an array of char and o an offset that misaligns it
// -- a global or local array, a variable or a call's argument -- once, at
// -O0 and -O2.  The address of an element and a pointer plus an offset are
// the controls.  A binding checks the referenced type's alignment, so it
// was reported here before (a load or store through the same pointer was
// not: CLANG-810, p3100-align-char-array-cast.cpp).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-align-binding-arith*.C
// (GCC-876: there the front end folded the binding and checked the array's
// address alone).

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
alignas(8) char buf[16];
// A global array plus an offset.
NI bool garr(int o) {
  int &r = *(int *)(buf + o);
  return &r == nullptr;
}
// A local array plus an offset.
NI bool larr(int o) {
  alignas(8) char lb[16];
  int &r = *(int *)(lb + o);
  return &r == nullptr;
}
// A call's argument.
NI void take(const int &) {}
NI void garg(int o) { take(*(int *)(buf + o)); }
// Controls: the address of an element, and a pointer plus an offset.
NI bool gsub(int o) {
  int &r = *(int *)&buf[o];
  return &r == nullptr;
}
NI bool gptr(char *c, int o) {
  int &r = *(int *)(c + o);
  return &r == nullptr;
}

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int volatile one = 1, four = 4;
  COUNT("garr", garr(one));
  COUNT("larr", larr(one));
  COUNT("garg", garg(one));
  COUNT("gsub", gsub(one));
  COUNT("gptr", gptr(buf, one));
  COUNT("aligned", garr(four); larr(four); garg(four); gsub(four);
        gptr(buf, four));
  return 0;
}

// CHECK: garr 1
// CHECK-NEXT: larr 1
// CHECK-NEXT: garg 1
// CHECK-NEXT: gsub 1
// CHECK-NEXT: gptr 1
// CHECK-NEXT: aligned 0
