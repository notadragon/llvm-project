// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fsyntax-only -verify %s
// expected-no-diagnostics

// P4405 in C: -fsyntax-only emits no checks, so prove and diagnose
// diagnose nothing.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-syntax-only.c

int f(int x)
  _Pre(x > 0)
{
  _ContractAssert(x != 3);
  return x;
}
