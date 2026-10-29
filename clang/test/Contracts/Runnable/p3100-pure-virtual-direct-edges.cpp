// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.indirect:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual:enforce \
// RUN:   %libcxx_flags -o %t2 && %t2 2>&1 | FileCheck %s
// P3100: a pure virtual function called without qualification in its class's
// constructor or destructor.  GCC resolves such a call statically and checks
// it as ub:class.abstract.pure.virtual.direct; Clang dispatches it through
// the vtable, whose slot is the pure-virtual terminus, so it is reported as
// the .indirect leaf (or the parent group), at -O0 and -O2 alike, under a
// throwing "enforce", and the definition never runs: a call by name, through
// `this`, through `*this`, in a destructor, in a class that does not override
// it, and in a class template.  A qualified call (which runs the definition),
// a call of a virtual function that is not pure, a call whose final overrider
// is not pure though name lookup found a pure virtual, and a call on another
// object are not reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-pure-virtual-direct-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
static int viols, defs;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
  throw Boom{};
}

struct B {
  B(int k) {
    switch (k) {
    case 1: f(); break;
    case 2: this->f(); break;
    case 3: (*this).f(); break;
    case 4: B::f(); break; // qualified: not checked
    case 5: g(); break;    // not pure
    }
  }
  virtual void f() = 0;
  virtual void g() {}
  int k = 0;
  virtual ~B() noexcept(false) {
    if (k == 6)
      f();
  }
};
void B::f() { ++defs; }
struct D : B {
  D(int k) : B(k) {}
  void f() override {}
};

// C does not override f, so its final overrider in C is still B::f.
struct C : B {
  C(int k) : B(0) {
    if (k == 7)
      f();
  }
};
struct E : C {
  E(int k) : C(k) {}
  void f() override {}
};

// Name lookup finds B1::f, but the final overrider in Q is B2::f.
struct B1 {
  virtual void f() = 0;
  virtual ~B1() {}
};
struct B2 : B1 {
  void f() override {}
};
struct Q : B2 {
  using B1::f;
  Q() { f(); }
};

void B1::f() { ++defs; }

template <typename T> struct TB {
  TB() { f(); }
  virtual void f() = 0;
  virtual ~TB() {}
};
template <typename T> void TB<T>::f() { ++defs; }
struct TD : TB<int> {
  void f() override {}
};

// Another object, of a class that overrides f: an ordinary virtual call.
struct O : B {
  O(B &other) : B(0) { other.f(); }
  void f() override {}
};

static void run(void (*fn)()) {
  try {
    fn();
  } catch (Boom &) {
  }
}

int main() {
  run([] { D d(4); });
  run([] { D d(5); });
  run([] { Q q; });
  run([] {
    D other(0);
    O o(other);
  });
  std::printf("clean=%d defs=%d\n", viols, defs);
  run([] { D d(1); });
  run([] { D d(2); });
  run([] { D d(3); });
  run([] {
    D d(0);
    d.k = 6;
  });
  run([] { E e(7); });
  run([] { TD t; });
  std::printf("caught=%d defs=%d\n", viols, defs);
  return 0;
}
// CHECK: clean=0 defs=1
// CHECK-NEXT: caught=6 defs=1
