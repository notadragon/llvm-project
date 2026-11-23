// RUN: %clang -std=c++26 -fsanitize=null -fsanitize-noncontract-callbacks -fno-contracts-p3100 -fcontracts-p3100 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s
// RUN: %clang -std=c++26 -fsanitize=null -fcontracts-p3100 -fsanitize-noncontract-callbacks -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks only
// means anything when sanitizer checks are routed to the contract-violation
// handler, like -fsanitize-semantic= (p3100-sanitize-semantic-no-p3100.cpp).
// No warning when -fcontracts-p3100 ends up on, whether given before or
// after it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-noncontract-callbacks-p3100-on.C

// CHECK-NOT: {{warning|error}}:

int f(int *p) { return *p; }
