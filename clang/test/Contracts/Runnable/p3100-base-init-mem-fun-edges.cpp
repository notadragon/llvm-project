// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a member function called on (*this) before the bases are
// initialized or after they are destroyed (ub:class.base.init.mem.fun) under
// "observe": a non-virtual call in a base-class or delegating
// mem-initializer, by name, through `this` or through (*this), in a
// constructor precondition and in a destructor postcondition, and in a class
// template's, is caught; a call in a member mem-initializer or the
// constructor body (a contract_assert there included), a static member
// function, a call on another object, a constructor postcondition, a
// destructor precondition and a lambda's captured `this` are not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-base-init-mem-fun-edges.C)

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
  int j;
  int f() const { return 7; } // reads no member
  static int sf() { return 7; }
  D() : Base(f()), j(f()) {}                                // 1
  D(int) : Base(this->f()), j(0) {}                         // 1
  D(long) : Base((*this).f()), j(0) {}                      // 1
  D(char) : D(f()) {}                                       // 1, then D(int)'s
  D(short) : Base(sf()), j(f()) {                           // 0
    f();
    contract_assert(this->f() == 7);
  }
  D(const D &o) : Base(o.f()), j(0) {}                      // 0
  D(double) : Base([this] { return f(); }()), j(0) {}       // 0
  D(bool) pre(this->f() == 7) post(this->f() == 7) : Base(0), j(0) {} // 1
  ~D() pre(this->f() == 7) post(this->f() == 7) {}          // 1
};
template <typename T> struct TD : Base {
  int f() const { return 7; }
  TD() pre(this->f() == 7) : Base(f()) {} // 2
};
int main() {
  {
    D a;
    D b(1);
    D c(1L);
    D d('c');
  }
  std::printf("mem-init=%d\n", viols);
  {
    D e((short)1);
    D g(e);
    D h(1.0);
  }
  std::printf("clean=%d\n", viols);
  {
    D k(true);
  }
  std::printf("contract=%d\n", viols);
  {
    TD<int> t;
  }
  std::printf("template=%d\n", viols);
  return 0;
}
// Every D destroyed runs ~D's postcondition: 4 + 3 + 1 objects of D.
// CHECK: mem-init=9
// CHECK-NEXT: clean=12
// CHECK-NEXT: contract=14
// CHECK-NEXT: template=16
