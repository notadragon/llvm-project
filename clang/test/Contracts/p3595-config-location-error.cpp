// RUN: not %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-configuration-file=%S/p3595-config-location-error.json \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 | FileCheck %s

// P3595 x config: a malformed line-range list in a "location" match criterion
// (":10x" trailing junk, ":1-2z" trailing junk after a range, ":5-" a range
// with no upper bound) is diagnosed against the configuration and its entry
// skipped, as GCC does, and does not send the parser into an infinite loop.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-config-location-error.C.)

// CHECK-COUNT-3: error: malformed line range
// CHECK-NOT: error:

int f(int x) { return x; }

// REQUIRES: contracts-libcxx
