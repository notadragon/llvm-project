// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -O2 -fcontract-analysis-semantic=prove %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function", DECISIONS.md E144 (a)): under
// -fcontract-analysis-semantic=prove every precondition is analyzed with
// prove, but one without a label allows runtime semantics, so taking its
// address -- including binding it to the forwarding reference of
// std::invoke or std::function -- is not diagnosed: this translation
// unit's configuration does not determine what calls through the address
// are.  Its calls through the pointer go unanalyzed and are still checked
// by the callee; a direct call is analyzed (and proven here).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-indirect-configured.C

// expected-no-diagnostics

#include <functional>

void fn(int x) pre(x > 0);
void fq(int x);

void
use()
{
  std::invoke(fn, 1);
  std::function<void(int)> g = fn;
  void(*p) (int) = fn;
  std::invoke(fq, 1);
  fn(1);
  g(1);
  p(1);
}
