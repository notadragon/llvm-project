// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:observe,ub:expr.assign.overlap:observe,ub:class.abstract.pure.virtual:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:observe,ub:expr.assign.overlap:observe,ub:class.abstract.pure.virtual:observe \
// RUN:   %libcxx_flags -o %t2 && %t2 2>&1 | FileCheck %s
// P3100: a run-time check inside a constexpr function stays when the call is
// not manifestly constant-evaluated, even where the call could be folded to a
// constant: the constructor call initializing an automatic variable, or a
// call in an initializer.  Under observe, at -O0 and -O2, a member function
// called in a base mem-initializer (ub:class.base.init.mem.fun), a pure
// virtual function called in its class's constructor (Clang reports it as
// ub:class.abstract.pure.virtual.indirect, hence the parent group; GCC as
// .direct) and an assignment from an overlapping object
// (ub:expr.assign.overlap) are each reported once, as are the forms whose
// calls cannot be folded.  Manifestly constant evaluation is unchanged: a
// constexpr or static variable is constant-initialized without a report, and
// the static_asserts hold.  Unlike GCC (stock GCC-760), Clang rejects B() in
// a constant expression, so there is no static_assert for it.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-constexpr-fold-runtime-check.C
// and its -O2 variant; GCC dropped the folded forms' checks, GCC-797.)

#include <contracts>
#include <cstdio>
#include <cstring>
namespace cs = std::contracts;

static int reports;
struct PureVirtual {};
void handle_contract_violation(const cs::contract_violation &v) {
  ++reports;
  // A pure virtual call has no valid continuation (the observing terminus
  // aborts after the handler returns): leave by an exception instead.
  if (std::strstr(v.comment(), "pure virtual"))
    throw PureVirtual{};
}

struct Base {
  int v;
  constexpr Base(int y) : v(y) {}
};

// Foldable: f() is a constant, so is the whole constructor call.
struct Derived : Base {
  int j;
  constexpr int f() const { return 7; }
  constexpr Derived() : Base(f()), j(5) {}
};

// Control: the constructor depends on its argument.
struct Derived3 : Base {
  int j;
  constexpr int f() const { return 7; }
  constexpr Derived3(int x) : Base(f() + x), j(5) {}
};

struct A {
  int v;
  constexpr virtual int g() const = 0;
  constexpr A() : v(g()) {}
};
constexpr int A::g() const { return 1; }
struct B : A {
  constexpr int g() const override { return 2; }
};

struct A3 {
  int v;
  constexpr virtual int g() const = 0;
  constexpr A3(int x) : v(g() + x) {}
};
constexpr int A3::g() const { return 1; }
struct B3 : A3 {
  constexpr B3(int x) : A3(x) {}
  constexpr int g() const override { return 2; }
};

union U {
  int a;
  short b;
};
constexpr int overlap() {
  U u;
  u.a = 1;
  u.b = u.a;
  return u.b;
}
constexpr int overlap3(int x) {
  U u;
  u.a = x;
  u.b = u.a;
  return u.b;
}
struct O {
  int v;
  constexpr O() : v(overlap()) {}
};

static_assert(Derived().v == 7);
static_assert(overlap() == 1);

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    try {                                                                      \
      __VA_ARGS__;                                                             \
    } catch (PureVirtual &) {                                                  \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main(int argc, char **) {
  COUNT("base-init", Derived d; (void)d);
  COUNT("base-init-control", Derived3 d3(argc); (void)d3);
  COUNT("pure-virtual", B b; (void)b);
  COUNT("pure-virtual-control", B3 b3(argc); (void)b3);
  COUNT("overlap-call", int r = overlap(); (void)r);
  COUNT("overlap-ctor", O o; (void)o);
  COUNT("overlap-control", int r3 = overlap3(argc); (void)r3);
  COUNT("constexpr-var", constexpr Derived cd; constexpr O co; (void)cd;
        (void)co);
  COUNT("static-var", static Derived sd; static O so; (void)sd; (void)so);
  return 0;
}

// CHECK: base-init 1
// CHECK-NEXT: base-init-control 1
// CHECK-NEXT: pure-virtual 1
// CHECK-NEXT: pure-virtual-control 1
// CHECK-NEXT: overlap-call 1
// CHECK-NEXT: overlap-ctor 1
// CHECK-NEXT: overlap-control 1
// CHECK-NEXT: constexpr-var 0
// CHECK-NEXT: static-var 0
