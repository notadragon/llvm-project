// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s

// P4405 in C (an extension, with -fcontracts-p4405): every evaluation with
// the prove semantic is an error -- a precondition, a postcondition (once,
// at its declaration, not at each return) and an assertion.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-prove.c

int f(int x)
  _Pre(x > 0)                   // expected-error {{contract assertion cannot be proven}}
  _Post(r: r > 0)               // expected-error {{contract assertion cannot be proven}}
{
  _ContractAssert(x != 3);      // expected-error {{contract assertion cannot be proven}}
  if (x == 4)
    return 1;
  return x;
}
