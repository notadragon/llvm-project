// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405 in C: the analysis evaluation of a contract assertion is separate
// from its runtime evaluation, which keeps its semantic (here enforce), and
// comes first, so its proof never relies on the assertion's own check: an
// enforced check, whose violation does not return, would prove it.  A later
// assertion of the same predicate is proven by the earlier ones.  A C
// precondition's analysis evaluation is in its function's body, ahead of its
// check, not at each call as in C++ (notadragon_wg21 DECISIONS.md E143 (a)).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-analysis-order.c

void a(int x) {
  _ContractAssert(x > 0); // expected-warning {{contract assertion cannot be proven}}
  _ContractAssert(x > 0);
}

int b(int x)
  _Pre(x > 0)             // expected-warning {{contract assertion cannot be proven}}
  _Post(r: r > 0)
{
  return x;
}

int c(int x)
  _Post(r: r > 0)         // expected-warning {{contract assertion cannot be proven}}
{
  return x;
}

int d(int y) {
  return b(y);
}
