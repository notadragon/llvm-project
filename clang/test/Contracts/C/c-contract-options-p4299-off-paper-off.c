// RUN: %clang -std=c23 -fcontracts-p3100 -fno-contracts-p3100 -fno-contracts-p4299 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule in C, where the final
// values decide: -fcontracts-p3100 given positively and then negatively is
// off, so a final -fno-contracts-p4299 is no error.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4299-off-paper-off.c

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __clang_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
