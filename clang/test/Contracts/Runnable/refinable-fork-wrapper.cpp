// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -O2 -fno-inline %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/k.cpp -o %t/k.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -O2 -fno-inline %libcxx_flags -fcontract-evaluation-semantic=enforce -c %t/u.cpp -o %t/u.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/k.o %t/u.o %t/main.o -o %t/ku && %t/ku
//
// The refinable-body rule for P3097 wrappers: a function that calls the
// wrapper of a virtual function holds the evaluations the wrapper (internal,
// this translation unit's) makes, which another translation unit's
// definition of the function may make differently or not at all.
//
// u.cpp holds the caller and the definition of h it is compiled with: h's
// P3097 wrapper checks vf's postcondition with enforce, so the wrapper's
// return value -- and so h's -- is positive.  -fno-inline keeps the wrapper
// out of line.  k.cpp, linked first, holds the definition of h the linker
// keeps, whose wrapper ignores the postcondition.  use_h (bp, -5) must be
// 0: LLVM takes no fact from h's linkonce_odr definition (the wrapper's
// facts, exact, stay inside h).  Clang has no caller-side checking, so
// gnu_gcc's caller-side half of this test has no mirror.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-fork-wrapper.C

//--- k.cpp
struct B
{
  virtual int vf (int x) post (r: r > 0);
};

int B::vf (int x) { return x; }

[[gnu::noinline]] inline int h (B *p, int x)
{
  return p->vf (x);
}

int (*volatile keep_h) (B *, int) = h;
B b;
B *bp = &b;

//--- u.cpp
struct B
{
  virtual int vf (int x) post (r: r > 0);
};

[[gnu::noinline]] inline int h (B *p, int x)
{
  return p->vf (x);
}

int use_h (B *p, int y) { return h (p, y) > 0; }

//--- main.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::abort ();
}

struct B;
extern B *bp;
int use_h (B *, int);

int main ()
{
  if (use_h (bp, -5) != 0)
    std::abort ();
}
