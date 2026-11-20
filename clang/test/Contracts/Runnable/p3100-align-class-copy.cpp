// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O0 -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O2 -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O0 -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:noexcept_observe,ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t | FileCheck %s

// P3100: a class copied, assigned, initialized or passed by value from a
// misaligned `*(P *)(c + o)` is reported once by the alignment check
// (ub:basic.align.object.alignment.store), at -O0 and -O2, and with the
// alignment group and the null-dereference group configured (the binding's
// check also tests null).  A reference variable and a read through it later
// are reported twice, the binding and the read; a store to the object and a
// member read once.  As gnu_gcc reports since GCC-879 (DECISIONS.md E120).
// The forms are in p3100-align-class-copy.h.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-align-class-copy{,-O2,-null}.C)

#include "p3100-align-class-copy.h"

// CHECK: gcopy 1
// CHECK-NEXT: pcopy 1
// CHECK-NEXT: passign 1
// CHECK-NEXT: pinit 1
// CHECK-NEXT: pbrace 1
// CHECK-NEXT: pvar 1
// CHECK-NEXT: parg 1
// CHECK-NEXT: passignto 1
// CHECK-NEXT: pref 2
// CHECK-NEXT: pmem 1
// CHECK-NEXT: aligned 0
