// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify %s
// expected-no-diagnostics

// Under observe, a speculative evaluation that is not manifestly constant --
// here the unused-variable check's -- reports no contract violation: the
// evaluation is just not constant.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/observe-warning-unused-var-eval.C

struct N { bool r = [](int x) pre(x > 5) { return true; }(1); };

void use() {
  N n;
}
