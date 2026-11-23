// RUN: %clang -std=c++26 -fsanitize=null -fcontracts-p3100 -fsanitize-noncontract-callbacks -fno-contracts-p3100 -fsyntax-only %s 2>&1 | FileCheck %s

// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks only
// means anything when sanitizer checks are routed to the contract-violation
// handler, like -fsanitize-semantic= (p3100-sanitize-semantic-no-p3100.cpp).
// It warns when -fcontracts-p3100 ends up off, here through a later
// -fno-contracts-p3100.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-noncontract-callbacks-p3100-off-after.C

// CHECK: warning: '-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'
// CHECK-NOT: {{warning|error}}:

int f(int *p) { return *p; }
