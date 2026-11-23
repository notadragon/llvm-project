// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=PARENT
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=PARENT
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=STORE

// P3100: the placement-new alignment check of a new-expression that stores
// nothing is the .nostore leaf of ub:basic.align.object.alignment: selected
// by the leaf and by the group (one trap, and no other check of the same
// pointer), and not by the .store leaf.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-placement-align-parent.C and
// p3100-placement-align-split-store.C)

void *operator new(decltype(sizeof 0), void *p) noexcept;
// PARENT-LABEL: define{{.*}} @_Z4makePv(
// PARENT: call void @llvm.trap()
// PARENT-NOT: call void @llvm.trap()
// PARENT: ret ptr
// STORE-LABEL: define{{.*}} @_Z4makePv(
// STORE-NOT: call void @llvm.trap()
// STORE: ret ptr
int *make(void *p) { return new (p) int; }
