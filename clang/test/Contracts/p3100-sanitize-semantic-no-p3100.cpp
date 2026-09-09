// -fsanitize-semantic= only means anything when checks are routed to the
// contract-violation handler.  On its own it was accepted and did nothing,
// which reads as an ordinary sanitizer option that quietly failed.
//
// The diagnostic has to come from the driver: the driver only renders
// -fsanitize-semantic= onward when routing is on, so cc1 never sees the
// flag in exactly the case worth warning about.
//
// GCC mirror: g++.dg/contracts/cpp26/p3100-sanitize-semantic-no-p3100.C
// .

// RUN: %clangxx -std=c++26 %s -fsanitize=null -fsanitize-semantic=null:assume \
// RUN:   -fsyntax-only %libcxx_flags 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts-p3850 -fsanitize=null \
// RUN:   -fsanitize-semantic=null:assume -fsyntax-only %libcxx_flags 2>&1 \
// RUN:   | FileCheck %s --check-prefix=QUIET --allow-empty

int deref(int *p) { return *p; }

// CHECK: warning: '-fsanitize-semantic=' has no effect without '-fcontracts-p3100'
// QUIET-NOT: has no effect without
