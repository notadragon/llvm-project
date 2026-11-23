// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.mem.init:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a member operator function of (*this) called in a base-class
// mem-initializer (ub:class.base.init.mem.fun.nonpolymorphic.mem.init) under
// "observe": `(*this)[0]` and `*this == *this` are each caught, as a named
// member function call is; an operator applied to another object is not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-base-init-mem-fun-operator.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
struct Base {
  int v;
  Base(int y) : v(y) {}
};
struct D : Base {
  int operator[](int) const { return 1; }
  bool operator==(const D &) const { return true; }
  D(const D *o) : Base((*o)[0] + (*o == *o)) {}
  D() : Base((*this)[0] + (*this == *this)) {}
};
int main() {
  D a;
  std::printf("viols=%d\n", viols);
  D b(&a);
  std::printf("viols=%d\n", viols);
  return 0;
}
// CHECK: viols=2
// CHECK-NEXT: viols=2
