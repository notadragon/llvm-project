// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-configuration-file=%S/c-p4405-analysis-json.json -emit-llvm-only -verify %s

// D4405 in C: configuration entries with "analysis": true select the
// semantic of the analysis evaluation in a lookup of their own (the first
// entry, which has no "analysis", does not stop the precondition's from
// being prove), and its other entries are skipped by the runtime lookups,
// which are ignore.  (The configuration errors:
// c-p4405-analysis-config-errors.c.)
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-analysis-json.c

int f(int x)
  _Pre(x > 0)                   // expected-error {{contract assertion cannot be proven}}
  _Post(r: r > 0)
{
  _ContractAssert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}
