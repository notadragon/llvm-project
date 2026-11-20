// RUN: %clang -std=c23 -fcontracts-p4299 -fcontracts-p3100 -fsanitize-noncontract-callbacks -fno-contracts-p3100 -fsyntax-only %s 2>&1 | FileCheck %s
// RUN: %clang -std=c23 -fsanitize-noncontract-callbacks -fsyntax-only %s 2>&1 | FileCheck %s

// P3100 (DECISIONS.md E178 (3)): in C as in C++,
// -fsanitize-noncontract-callbacks only means anything when sanitizer
// checks are routed to the contract-violation handler.  Without
// -fcontracts-p3100 in effect, here turned off by a later
// -fno-contracts-p3100 or never given, it warns that it has no effect.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p3100-noncontract-callbacks-no-p3100.c

// CHECK: warning: '-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'
// CHECK-NOT: {{warning|error}}:

int f(int *p) { return *p; }
