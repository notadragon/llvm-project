// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O0 -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O2 -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t | FileCheck %s

// P3100: the forms of p3100-align-class-copy.cpp with -fsanitize=alignment
// routed to the handler and the P3100 leaf not configured: the sanitizer's
// checks are stock -fsanitize=alignment's (18.1.0 to trunk alike), at -O0 and
// -O2.  The copies, the assignments and the store are reported once; a
// reference variable and a read through it twice (two operations).  Pinned
// here as stock behaviour:
// * gcopy, a copy from the global character array `*(P *)(buf + o)`: 0, the
//   stock miss through a pointer formed from a character array (CLANG-809,
//   deferred by DECISIONS.md E127 (a));
// * pmem, the member read `(*(P *)(c + o)).a`: 2, the member access's check
//   of the object and the load's check of the member, both stock.  gnu_gcc's
//   sanitizer reports it once.  Accepted under DECISIONS.md E120 (CLANG-817):
//   the load's check is the only check where the object is not checked at
//   the member access (`r.a`, `r` a reference parameter), so dropping it
//   needs the member access's check carried to the load (extra state).
// (GCC mirror: g++.dg/ubsan/p3100-align-class-copy-sanitize.C, which reports
// the copies twice and the member read once.)

#include "p3100-align-class-copy.h"

// CHECK: gcopy 0
// CHECK-NEXT: pcopy 1
// CHECK-NEXT: passign 1
// CHECK-NEXT: pinit 1
// CHECK-NEXT: pbrace 1
// CHECK-NEXT: pvar 1
// CHECK-NEXT: parg 1
// CHECK-NEXT: passignto 1
// CHECK-NEXT: pref 2
// CHECK-NEXT: pmem 2
// CHECK-NEXT: aligned 0
