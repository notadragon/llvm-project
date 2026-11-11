// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Local Proofs": a precondition with the always_prove label must
// be proven at every call.  f(1) is proven by folding, the guarded call by
// the proof shadow (optimized), and the unguarded f(y) cannot be proven.
// The paper's example, with its comments.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-local-pre.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

void f(int x) pre<always_prove>(x > 0);  // expected-note {{the predicate of the contract assertion}}

void g(int y)
{
  f(1);  // OK, proven
  f(y);  // Error, cannot be proven expected-error {{contract assertion cannot be proven}}
  if (y > 0) {
    f(y);  // OK, proven
  }
}
