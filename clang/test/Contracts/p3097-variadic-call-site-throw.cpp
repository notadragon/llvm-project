// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A virtual call to a C-variadic function checked at the call site: when a
// postcondition's handler throws, the class result the call returned is
// destroyed ([except.ctor]/2).

#include <contracts>

static int ctors, dtors;
struct R {
  int v;
  R(int v) : v(v) { ++ctors; }
  R(const R &o) : v(o.v) { ++ctors; }
  ~R() { ++dtors; }
};

void handle_contract_violation(const std::contracts::contract_violation &) {
  throw 7;
}

struct B {
  virtual R make(int n, ...) post(r : r.v > 0);
};
R B::make(int n, ...) { return R(n); }

int main() {
  B b;
  try {
    R r = b.make(-1);
    (void)r;
  } catch (int) {
  }
  if (ctors != 1 || dtors != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
