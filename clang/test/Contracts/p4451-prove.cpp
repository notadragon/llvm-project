// DEFINE: %{cc} = %clangxx -std=c++26 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// RUN: %{cc} -fcontracts-p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451
// REQUIRES: contracts-libcxx

// P4451, D4405: a terminating contract assertion allows prove, as P3400's
// labels::terminating does: with the analysis evaluations configured prove
// its precondition must be proven at each call, and it is still enforced at
// run time.  p4405-paper-analysis-terminating.cpp under -fcontracts-p4451
// alone, -fcontracts-p3400 alone and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-prove.C (and its -both
// variant; p4405-paper-analysis-terminating.C is the -p3400 one)

#include <contracts>  // `terminating`

int table[101];

int lookup(int p)
  pre<terminating>(p >= 0 && p <= 100)  // expected-note {{the predicate of the contract assertion}}
{
  return table[p];
}

int lookup_clamped(int v)
{
  if (v < 0)
    v = 0;
  if (v > 100)
    v = 100;
  return lookup(v);  // OK, proven
}

int lookup_raw(int v)
{
  return lookup(v);  // Error if proven, otherwise enforced expected-error {{contract assertion cannot be proven}}
}
