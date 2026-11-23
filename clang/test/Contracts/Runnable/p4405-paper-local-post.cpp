// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -o %t -Xclang -verify
// RUN: %t | FileCheck %s
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// D4405R0, "Local Proofs": a postcondition with always_prove is proven
// against the function body (by the proof shadow, optimized).  The paper's
// example, with its comment.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-local-post.C

#include <cstdio>
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

int g(bool b)
  post<always_prove>(r : r > 0)  // OK, proven
{
  return b ? 17 : 34;
}

int main()
{
  std::printf("%d %d\n", g(true), g(false));
}

// CHECK: 17 34
