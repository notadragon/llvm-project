// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.polymorphic:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: the polymorphic operations on (*this) before the bases are
// initialized (ub:class.base.init.mem.fun.polymorphic) under "enforce", with
// a handler that throws to get back before the operation reads the
// uninitialized vptr: a virtual call, typeid and dynamic_cast in a
// base-class mem-initializer, and a virtual call in a constructor
// precondition, are caught; a qualified (non-virtual) call is the
// nonpolymorphic leaf, not configured here.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-base-init-mem-fun-poly.C)

#include <contracts>
#include <cstdio>
#include <typeinfo>
namespace cs = std::contracts;
struct Back {};
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  // A violation in a precondition's predicate is an exception from its
  // evaluation, reported again as the precondition's: throw past both.
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
  throw Back{};
}
struct Base {
  Base(bool) {}
};
struct D : Base {
  virtual int vf() const { return 7; }
  virtual ~D() = default;
  D(int) : Base(vf() == 7) {}
  D(long) : Base(&typeid(*this) != nullptr) {}
  D(char) : Base(dynamic_cast<const void *>(this) != nullptr) {}
  D(short) : Base(D::vf() == 7) {}
  D(bool) pre(this->vf() == 7) : Base(true) {}
};
int main() {
  try {
    D a(1);
  } catch (Back &) {
  }
  try {
    D b(1L);
  } catch (Back &) {
  }
  try {
    D c('c');
  } catch (Back &) {
  }
  try {
    D e((short)1);
  } catch (Back &) {
  }
  try {
    D d(true);
  } catch (Back &) {
  }
  std::printf("viols=%d\n", viols);
  return 0;
}
// CHECK: viols=4
