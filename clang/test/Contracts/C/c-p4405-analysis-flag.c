// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose -emit-llvm-only -verify %s

// D4405 in C: -fcontract-analysis-semantic= selects the semantic of the
// analysis evaluation of every contract assertion, separately from the
// runtime semantic (here ignore): diagnose warns for each one that cannot
// be proven.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-analysis-flag.c

int f(int x)
  _Pre(x > 0)                   // expected-warning {{contract assertion cannot be proven}}
  _Post(r: r > 0)               // expected-warning {{contract assertion cannot be proven}}
{
  _ContractAssert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}
