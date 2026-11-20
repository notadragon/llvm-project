// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -DVIOLATE=0 -o %t0
// RUN: %t0 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -DVIOLATE=1 -o %t1
// RUN: not --crash %t1
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -DVIOLATE=2 -o %t2
// RUN: not --crash %t2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -DVIOLATE=3 -o %t3
// RUN: not --crash %t3
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -DVIOLATE=4 -o %t4
// RUN: not --crash %t4

// A precondition or postcondition naming `this', implicitly or explicitly,
// on a member function declared in the class and defined outside it is
// checked against the object it is called on; so is the precondition of a
// destructor defined outside the class, and of one defined in it, when the
// destructor runs.  VIOLATE=0 satisfies every check; each other value fails
// one, which traps under quick_enforce.  (EDG-55 shape: codegen/this_param.)
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/this-param-out-of-class-run.C
// (observe, every violation in one run) and this-param-out-of-class-quick.C.

#include <cstdio>

struct T {
  int n;
  int get() const pre(n >= 0) post(r: r == this->n);
  void set(int v) pre(this->n != v);
  ~T() pre(n != 3);
};
int T::get() const { return n; }
void T::set(int v) { n = v; }
T::~T() {}

struct W {
  int value;
  ~W() pre(value >= 0) {}
};

int main() {
  if (VIOLATE == 0) { T t{1}; t.set(2); t.get(); W w{0}; }
  if (VIOLATE == 1) { T t{-1}; t.get(); }   // precondition, implicit this
  if (VIOLATE == 2) { T t{2}; t.set(2); }   // precondition, explicit this
  if (VIOLATE == 3) { T t{3}; }             // destructor's precondition
  if (VIOLATE == 4) { W w{-1}; }            // in-class destructor's
  std::puts("end");
}

// CHECK: end
