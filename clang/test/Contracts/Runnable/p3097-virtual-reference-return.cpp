// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: the interface wrapper of a virtual function returning a reference
// (lvalue, const lvalue or rvalue) must return the same object the final
// overrider does, not a copy or a temporary.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3097-virtual-reference-return.C

#include <contracts>

static int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct B {
  int m = 1;
  virtual int &get(int x) pre(x > 0) { return m; }
  virtual const int &cget(int x) const pre(x > 0) { return m; }
  virtual int &&rget(int x) pre(x > 0) { return static_cast<int &&>(m); }
  virtual ~B() {}
};

struct D : B {
  int n = 2;
  int &get(int) override { return n; }
  const int &cget(int) const override { return n; }
  int &&rget(int) override { return static_cast<int &&>(n); }
};

int main() {
  D d;
  B &b = d;
  if (&b.get(1) != &d.n || &b.cget(1) != &d.n || violations != 0)
    __builtin_abort();
  b.get(0) = 5;
  if (d.n != 5 || violations != 1)
    __builtin_abort();
  int &&r = b.rget(0);
  if (&r != &d.n || violations != 2)
    __builtin_abort();
}
