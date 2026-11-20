// RUN: %clang -std=c++26 -fsanitize=null -fsanitize-noncontract-callbacks -fsyntax-only %s 2>&1 | FileCheck %s

// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks only
// means anything when sanitizer checks are routed to the contract-violation
// handler, like -fsanitize-semantic= (p3100-sanitize-semantic-no-p3100.cpp).
// Without -fcontracts-p3100 it warns that it has no effect.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-noncontract-callbacks-no-p3100.C

// CHECK: warning: '-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'
// CHECK-NOT: {{warning|error}}:

int f(int *p) { return *p; }
