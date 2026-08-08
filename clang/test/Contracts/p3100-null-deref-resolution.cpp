// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-resolution.json %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// P3100: the implicit null-dereference semantic is resolved *per site* from the
// P3595 configuration, distinguishing source location (line range) and
// namespace, first-match-wins.  We verify this by which functions get a trap
// (quick_enforce) vs a plain load (assume).
//
// Config (p3100-null-deref-resolution.json), first match wins:
//   1. namespace "assume_ns"              -> assume         (no trap)
//   2. location lines 22-26 of this file  -> quick_enforce  (trap)
//   3. default                            -> assume         (no trap)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-resolution.C)

#include <contracts>

// Out of the quick line-range, not in assume_ns: default -> assume, no trap.
int g_out(int *p) { return *p; }

namespace assume_ns {
// In assume_ns AND in the quick line range (22-26): the namespace entry is first,
// so it wins -> assume, no trap (first-match precedence over the line entry).
int a_in(int *p) { return *p; }
}

// In the quick line range (line 26), not in assume_ns: line entry matches -> quick_enforce, trap.
int g_in(int *p) { return *p; }

// CHECK-LABEL: define {{.*}}g_out
// CHECK-NOT: @llvm.trap
// CHECK-LABEL: define {{.*}}a_in
// CHECK-NOT: @llvm.trap
// CHECK-LABEL: define {{.*}}g_in
// CHECK: @llvm.trap
