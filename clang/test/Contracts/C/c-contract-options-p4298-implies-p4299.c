// RUN: %clang -std=c23 -fcontracts-p4298 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule in C, where
// -fcontracts-p4298 turns -fcontracts-p4299 on as a C++ per-paper option
// turns -fcontracts on.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4298-implies-p4299.c

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifndef __clang_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f(int x) _Pre(x > 0) { return x; }
