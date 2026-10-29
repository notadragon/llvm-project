// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3 in C, -fcontracts-prove-shadow (implied by
// -fcontracts-p4405): at -O2 a contract assertion, a precondition or a
// postcondition the proof shadow proves is not diagnosed, one it does not
// is.  The postcondition is proven at every return (c-p4405-shadow-post.c).
// c-p4405-shadow-O0.c has -O0.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-shadow-prove.c

static int clamp(int v) { return v < 0 ? 0 : v > 9 ? 9 : v; }

int f(int i, int n)
  _Pre(n >= 0)                  // expected-error {{contract assertion cannot be proven}}
  _Post(r: r >= 0)
{
  if (i >= 0 && i < n)
    _ContractAssert(i < n);
  int c = clamp(i);
  _ContractAssert(c <= 9);
  _ContractAssert(c < 9);       // expected-error {{contract assertion cannot be proven}}
  return c;
}
