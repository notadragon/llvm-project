// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p4283 \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 | FileCheck %s

// P4283 x P3097 -- KNOWN DIVERGENCE (cross-compiler-mirror #18): GCC honors a
// requires clause on a virtual function's contract (an unsatisfied constraint
// discards the contract so the interface wrapper omits it).  Clang does not
// implement this: it deliberately diagnoses a requires clause on a virtual
// function's contract.  This test pins Clang's actual behaviour so that, if
// Clang ever gains the feature, the change is caught.
// (GCC mirror behaviour: g++.dg/contracts/cpp26/p4283-virtual-discard.C.)

#include <concepts>
#include <contracts>

template <class T>
struct Base {
  // CHECK: virtual function cannot have a requires clause
  virtual void f(T x) pre requires (std::integral<T>) (x > 0) { }
  virtual ~Base() = default;
};

template struct Base<int>;
