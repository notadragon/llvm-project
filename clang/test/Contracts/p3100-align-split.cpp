// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=GUARDED
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=GUARDED
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=UNGUARDED

// P3100: every access-site alignment check is the .store leaf of
// ub:basic.align.object.alignment (a creation that performs no store,
// .nostore, is checked only at a placement new:
// p3100-placement-align-split.cpp).  A misaligned access is guarded when
// .store or the parent group is configured, and not when only .nostore is.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-align-split-store.C and
// p3100-align-split-nostore.C)

// GUARDED-LABEL: define{{.*}} @_Z4loadPi(
// GUARDED: call void @llvm.trap()
// UNGUARDED-LABEL: define{{.*}} @_Z4loadPi(
// UNGUARDED-NOT: call void @llvm.trap()
int load(int *p) { return *p; }
