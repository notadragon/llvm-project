// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405 ("The Analysis Evaluation"): the analysis evaluation is the first
// evaluation of the assertion, so its proof never relies on the
// assertion's own runtime check.  Each assertion below is checked with
// enforce, whose violation does not return, and analyzed with diagnose: had
// the analysis come after the check, the check would prove it.  A later
// assertion of the same predicate is proven by the earlier ones.  At a call
// the analysis is the precondition's first evaluation too (Clang has no
// caller-side checking; the callee's check is in the callee).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-order.C
// (which adds -fcontracts-client-check=all)

void a(int x) {
  contract_assert(x > 0); // expected-warning {{contract assertion cannot be proven}}
  contract_assert(x > 0);
}

int b(int x)
  post(r: r > 0) // expected-warning {{contract assertion cannot be proven}}
{
  return x;
}

int c(int x) pre(x > 0); // expected-note {{the predicate of the contract assertion}}

int d(int y) {
  int r = c(y); // expected-warning {{contract assertion cannot be proven}}
  return r + c(y);
}
