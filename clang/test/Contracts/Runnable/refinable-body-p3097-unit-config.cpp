// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -O2 %libcxx_flags -fcontract-evaluation-semantic=enforce -c %t/k.cpp -o %t/k.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/u.cpp -o %t/u.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/k.o %t/u.o %t/main.o -o %t/ku
// RUN: %t/ku throwing
// RUN: %t/ku noexcept
//
// DECISIONS.md E136 in Clang: whether a call is checked outside the callee
// is decided per translation unit.  Clang has no caller-side checking; its
// only such checks are P3097's, for a virtual call, made where the interface
// contracts are active: k.cpp's g calls B::f through the wrapper, which
// checks the precondition with enforce, while u.cpp, which ignores it, has
// no wrapper, so its g is not marked (refinable bodies) and may be found
// nounwind.  The linker keeps k.cpp's g.  This cannot go wrong: u.cpp's g
// is found nounwind only if its virtual call is, that is, if B::f is
// noexcept, and then the wrapper's throwing handler terminates in k.cpp's g
// too (p3097-noexcept-caller-side-terminate.cpp); otherwise the call may
// throw and use_g keeps its handler.  No other fact reaches use_g: LLVM
// takes none from g's linkonce_odr definition.  So gnu_gcc's GCC-904
// (caller-side checking) has no Clang form.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-caller-side-unwrapped.C
// (the gnu_gcc form, xfailed there).

//--- h.h
struct B {
  virtual int f(int x) pre(x > 0);
  virtual int nf(int x) noexcept pre(x > 0);
  virtual ~B() = default;
};

[[gnu::noinline]] inline int g(B &b, int x) { return b.f(x); }
[[gnu::noinline]] inline int ng(B &b, int x) { return b.nf(x); }

//--- k.cpp
// The definitions of g and ng the linker keeps: their wrappers check the
// preconditions with enforce.
#include "h.h"

int (*volatile keep_g)(B &, int) = g;
int (*volatile keep_ng)(B &, int) = ng;

//--- u.cpp
// The preconditions ignored: no wrapper, so g and ng are not marked.  ng's
// call of a noexcept function cannot throw, so ng is nounwind and use_ng
// drops its handler; g's may, so use_g keeps its.
#include "h.h"

int B::f(int x) { return x; }
int B::nf(int x) noexcept { return x; }

int use_g(int y) {
  B b;
  try {
    return g(b, y);
  } catch (...) {
    return -1;
  }
}

int use_ng(int y) {
  B b;
  try {
    return ng(b, y);
  } catch (...) {
    return -1;
  }
}

//--- main.cpp
#include <contracts>
#include <cstdlib>
#include <cstring>
#include <exception>

void handle_contract_violation(const std::contracts::contract_violation &) {
  throw 42;
}

int use_g(int);
int use_ng(int);

static void on_terminate() { std::_Exit(0); }

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;
  if (!std::strcmp(argv[1], "throwing"))
    return use_g(-5) == -1 ? 0 : 1; // caught in use_g
  // A throwing handler under a noexcept function terminates, whichever
  // definition of ng runs.
  std::set_terminate(on_terminate);
  use_ng(-5);
  return 1;
}
