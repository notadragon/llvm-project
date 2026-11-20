// RUN: not %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/malformed-location-range.json \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s

// A malformed line range in a P3595 "location" match criterion is diagnosed
// and its entry skipped, as GCC does: "10x" (trailing junk), "1-2z" (junk
// after a range) and "5-" (no upper bound).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/malformed-location-range.C

// CHECK: error: {{.*}}malformed line range 'err.cpp:10x'
// CHECK: error: {{.*}}malformed line range 'err.cpp:1-2z'
// CHECK: error: {{.*}}malformed line range 'err.cpp:5-'

int f(int x) { return x; }
