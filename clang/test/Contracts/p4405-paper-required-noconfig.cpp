// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// D4405R0, "Proving Preconditions": p4405-paper-required.cpp without its
// configuration -- the analysis evaluations are ignore, and nothing is
// diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-required-noconfig.C

#include <contracts>  // `""group`

void f(int x) pre<"required"group>(x > 0);

void g()
{
  f(1);  // OK
  f(0);  // Error if the `"required"` group has the *prove* semantic
}
