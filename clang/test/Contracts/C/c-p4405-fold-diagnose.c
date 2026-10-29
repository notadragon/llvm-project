// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify %s

// P4405 stage 2 in C: as c-p4405-fold-prove.c with the diagnose semantic --
// a -Wcontract-unproven warning, for "always false" as for "cannot be
// proven", and nothing for a predicate that folds to true.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-fold-diagnose.c

int f(int x)
  _Pre(1)
  _Pre(x > 0)                   // expected-warning {{contract assertion cannot be proven}}
  _Post(r: sizeof(r) == 0)      // expected-warning {{contract predicate is always false}}
{
  _ContractAssert(0);           // expected-warning {{contract predicate is always false}}
  _ContractAssert(2 > 1);
  return x;
}
