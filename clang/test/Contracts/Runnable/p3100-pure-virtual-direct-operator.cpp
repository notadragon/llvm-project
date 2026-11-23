// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.indirect:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a pure virtual operator of *this called in its class's constructor
// under a throwing "enforce": an operator() call, a subscript, a unary and a
// binary operator, a compound assignment and a conversion function.  GCC
// resolves each statically and checks it as
// ub:class.abstract.pure.virtual.direct (GCC-772); Clang dispatches each
// through the vtable to the pure-virtual terminus, so they are reported as
// the .indirect leaf, as a named call is (p3100-pure-virtual-direct-edges.cpp),
// and the definition never runs.  The qualified form (which runs the
// definition) and an operator that is not pure are not reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-pure-virtual-direct-operator.C)

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
    case 1: (*this)(); break;
    case 2: (void)(*this)[0]; break;
    case 3: (void)-*this; break;
    case 4: (void)(*this + 1); break;
    case 5: *this += 1; break;
    case 6: (void)static_cast<int>(*this); break;
    case 7: B::operator()(); break; // qualified: not checked
    case 8: (void)!*this; break;    // not pure
    }
  }
  virtual void operator()() = 0;
  virtual int operator[](int) = 0;
  virtual int operator-() = 0;
  virtual int operator+(int) = 0;
  virtual B &operator+=(int) = 0;
  virtual operator int() = 0;
  virtual bool operator!() { return false; }
  virtual ~B() {}
};
void B::operator()() { ++defs; }
int B::operator[](int) { return ++defs; }
int B::operator-() { return ++defs; }
int B::operator+(int) { return ++defs; }
B &B::operator+=(int) {
  ++defs;
  return *this;
}
B::operator int() { return ++defs; }

struct D : B {
  D(int k) : B(k) {}
  void operator()() override {}
  int operator[](int) override { return 0; }
  int operator-() override { return 0; }
  int operator+(int) override { return 0; }
  D &operator+=(int) override { return *this; }
  operator int() override { return 0; }
};

static void run(int k) {
  try {
    D d(k);
  } catch (Boom &) {
  }
}

int main() {
  run(7);
  run(8);
  std::printf("clean=%d defs=%d\n", viols, defs);
  for (int k = 1; k <= 6; ++k)
    run(k);
  std::printf("caught=%d defs=%d\n", viols, defs);
  return 0;
}
// CHECK: clean=0 defs=1
// CHECK-NEXT: caught=6 defs=1
