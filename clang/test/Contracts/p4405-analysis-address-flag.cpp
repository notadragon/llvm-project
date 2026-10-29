// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -fsyntax-only -Werror=contract-unproven -verify %s

// D4405 ("Taking the Address of a Function"): with
// -fcontract-analysis-semantic=prove and no label, every precondition
// allows a runtime semantic, which the callee evaluates as its own
// translation unit configures, so taking the address of the function is
// not diagnosed, not even under -Werror: the configuration of the
// translation unit taking the address does not determine what calls
// through it are.  A call through the pointer is not analyzed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-address-flag.C

// expected-no-diagnostics

int f(int x) pre(x > 0);
int g(int x);

int use(int y) {
  int (*p)(int) = f;
  int (*q)(int) = g;
  int (&r)(int) = f;
  return p(y) + q(y) + r(y);
}
