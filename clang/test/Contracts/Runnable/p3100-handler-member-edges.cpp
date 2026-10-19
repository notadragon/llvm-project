// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-exceptions \
// RUN:   -fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a member referred to in a constructor or destructor
// function-try-block handler (ub:except.handle.handler.ctor.dtor) under
// "observe": a data member, a member of a base, a member function and a
// base's member function are caught, by name or through `this`, in a
// constructor's handler and in a destructor's; a static member, a member of
// another object, an unevaluated operand, a handler of an ordinary try block
// or of another member function, and a lambda's captured `this` are not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-handler-member-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
static void use(const void *) {}
struct P {
  long pad;
};
struct B {
  int b;
  int g() const { return 1; }
};
struct A : P, B {
  int x;
  int y = 42;
  static int s;
  static int sf() { return 2; }
  int f() const { return 3; }
  A(int v, const A *other) try : x(v == 1 ? throw 1 : 1) {
    try {
      throw 2;
    } catch (int) {
      use(&y); // not a function-try-block
    }
  } catch (int) {
    std::printf("before=%d\n", viols);
    use(&s);
    sf();
    use(&other->y);
    use(&v);
    (void)sizeof(y);
    [this] { use(&y); }(); // through the captured this: not checked
    std::printf("clean=%d\n", viols);
    use(&y);
    use(&this->y);
    use(&b);
    f();
    this->f();
    g();
    std::printf("ctor=%d\n", viols);
  }
  ~A() noexcept(false) try {
    if (x == 7)
      throw 3;
  } catch (int) {
    use(&y);
    f();
  }
  void m() try { throw 4; } catch (int) {
    use(&y);
    f();
  }
};
int A::s;
int main() {
  A other(0, nullptr);
  other.m();
  try {
    A a(1, &other);
  } catch (int) {
  }
  std::printf("m=%d\n", viols);
  try {
    A *d = new A(0, nullptr);
    d->x = 7;
    delete d;
  } catch (int) {
  }
  std::printf("dtor=%d\n", viols);
  return 0;
}
// CHECK: before=0
// CHECK-NEXT: clean=0
// CHECK-NEXT: ctor=6
// CHECK-NEXT: m=6
// CHECK-NEXT: dtor=8
