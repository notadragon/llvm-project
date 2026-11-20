// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// When a violation handler throws from the static type's postcondition in a
// P3097 wrapper, the already initialized returned object is destroyed
// ([except.ctor]/2).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/retval-leak-throwing-post.C

#include <contracts>

static int ctor = 0, dtor = 0;
struct R {
  R() { ++ctor; }
  R(const R &) { ++ctor; }
  ~R() { ++dtor; }
};

void handle_contract_violation(const std::contracts::contract_violation &) {
  throw 7;
}

// Only the interface (Base) postcondition fails; the overrider's passes.
struct Base { virtual R f(const int x) post(x < 0); };
R Base::f(const int) { return R(); }
struct Der : Base { R f(const int x) override post(x > 0); };
R Der::f(const int) { return R(); }
Der d;
Base &rb = d;

int main() {
  try { R r = rb.f(1); } catch (int) {}
  if (ctor != 1 || dtor != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
