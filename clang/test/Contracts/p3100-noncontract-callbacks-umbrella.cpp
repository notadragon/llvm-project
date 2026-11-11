// RUN: %clang -std=c++26 -fsanitize=null -fsanitize-noncontract-callbacks -fcontracts-p3850 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks only
// means anything when sanitizer checks are routed to the contract-violation
// handler, like -fsanitize-semantic= (p3100-sanitize-semantic-no-p3100.cpp).
// No warning when -fcontracts-p3850 turns -fcontracts-p3100 on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-noncontract-callbacks-umbrella.C

// CHECK-NOT: {{warning|error}}:

int f(int *p) { return *p; }
