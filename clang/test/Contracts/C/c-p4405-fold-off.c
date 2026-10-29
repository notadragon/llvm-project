// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-fold -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %clang -### -fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-fold -fno-contracts-prove-assume -c %s 2>&1 | FileCheck %s --check-prefix=DRIVER

// P4405 in C: -fno-contracts-prove-fold gives stage 1's behaviour back:
// every evaluation with prove is an error, "cannot be proven", even when its
// predicate is true or false.  The driver passes both strategy flags on in
// C too.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-fold-off.c (which also
// switches GCC's proof shadow off; the driver line is Clang's own)

void f(void)
  _Pre(1)                       // expected-error {{contract assertion cannot be proven}}
{
  _ContractAssert(1);           // expected-error {{contract assertion cannot be proven}}
  _ContractAssert(0);           // expected-error {{contract assertion cannot be proven}}
}

// DRIVER: "-cc1"
// DRIVER-SAME: "-fno-contracts-prove-fold"
// DRIVER-SAME: "-fno-contracts-prove-assume"
