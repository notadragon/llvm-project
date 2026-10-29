// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-configuration-file=%S/p4405-paper-required.json %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Proving Preconditions": a configuration entry with the analysis
// key gives the analysis evaluations of the "required" group's
// preconditions the prove semantic, at each call; f(0)'s predicate is
// always false once the argument is substituted, and f(1) is proven.  The
// entry says nothing about the runtime evaluations.  The paper's example,
// with its comments, and its configuration
// (p4405-paper-required-noconfig.cpp is the same without it).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-required.C

#include <contracts>  // `""group`

void f(int x) pre<"required"group>(x > 0);  // expected-note {{the predicate of the contract assertion}}

void g()
{
  f(1);  // OK
  f(0);  // Error if the `"required"` group has the *prove* semantic expected-error {{contract predicate is always false}}
}
