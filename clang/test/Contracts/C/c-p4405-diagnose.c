// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify %s
// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-llvm-only -verify=quiet %s
// quiet-no-diagnostics

// P4405 in C: every evaluation with the diagnose semantic gets a
// -Wcontract-unproven warning, on by default; -Wno-contract-unproven
// silences it.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-diagnose.c

int f(int x)
  _Pre(x > 0)                   // expected-warning {{contract assertion cannot be proven}}
  _Post(r: r > 0)               // expected-warning {{contract assertion cannot be proven}}
{
  _ContractAssert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}
