// RUN: %clang -std=c23 -fsanitize-noncontract-callbacks -fno-contracts-p3100 -fcontracts-p3100 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s
// RUN: %clang -std=c23 -fcontracts-p3100 -fsanitize-noncontract-callbacks -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// P3100 (DECISIONS.md E178 (3)): in C as in C++,
// -fsanitize-noncontract-callbacks only means anything when sanitizer
// checks are routed to the contract-violation handler.  With
// -fcontracts-p3100 on it does not warn, whether given before or after it.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p3100-noncontract-callbacks-p3100.c

// CHECK-NOT: {{warning|error}}:

int f(int *p) { return *p; }
