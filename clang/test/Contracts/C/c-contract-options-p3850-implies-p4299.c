// RUN: %clang -std=c23 -fcontracts-p3850 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E179 (1): -fcontracts-p3850 works on a C input.  It turns on
// the C per-paper options (-fcontracts-p3100, -fcontracts-p4298 and
// -fcontracts-p4405), and so -fcontracts-p4299, as it turns -fcontracts on in
// C++.  What it hands -cc1 is c-contract-options-p3850-driver.c.

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifndef __clang_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f(int x) _Pre(x > 0) { return x; }
