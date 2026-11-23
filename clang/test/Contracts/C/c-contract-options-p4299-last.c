// RUN: %clang -std=c23 -fno-contracts-p4299 -fcontracts-p3100 -fcontracts-p4299 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule in C, where the last of
// -fcontracts-p4299 and -fno-contracts-p4299 decides.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4299-last.c

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifndef __clang_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f(int x) _Pre(x > 0) { return x; }
