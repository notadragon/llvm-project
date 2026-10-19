// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.indirect:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=INDIRECT
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=INDIRECT
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=DIRECT

// P3100: the pure-virtual vtable slot is reached only by a dispatched call,
// the .indirect leaf of ub:class.abstract.pure.virtual; configuring that leaf
// (or the parent group) selects the terminus.  A direct call in the class's
// own constructor or destructor (.direct) is not checked, so configuring only
// that leaf leaves the slot at __cxa_pure_virtual.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-pure-virtual-split-indirect.C
// and p3100-pure-virtual-split-direct.C)

struct Base {
  virtual void f() = 0;
  virtual ~Base(); // out-of-line key function: emits Base's vtable here
};
Base::~Base() {}

// INDIRECT: @_ZTV4Base = {{.*}} @__cxa_pure_virtual_quick
// DIRECT: @_ZTV4Base = {{.*}} @__cxa_pure_virtual,
// DIRECT-NOT: __cxa_pure_virtual_quick
