// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.mem.init:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ONE
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.contract:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=TWO
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.polymorphic.virtual.call:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ONE
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.polymorphic.typeid:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ONE
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.polymorphic.dynamic.cast:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=ONE
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=THREE
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.polymorphic:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=THREE
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:quick_enforce \
// RUN:   | FileCheck %s --check-prefix=SIX

// P3100: each leaf of ub:class.base.init.mem.fun checks its own operation on
// (*this), one site each in the source below except .nonpolymorphic.contract,
// which has two (a constructor precondition and a destructor postcondition);
// the two subgroups and the parent select their leaves.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-base-init-mem-fun-split-*.C)

namespace std {
class type_info;
}
struct Base {
  Base(bool) {}
};
struct D : Base {
  int f() const { return 7; }
  virtual int vf() const { return 7; }
  ~D() pre(this->f() == 7) post(this->f() == 7) {}
  D(int) : Base(f() == 7) {}                         // .nonpolymorphic.mem.init
  D(long) pre(this->f() == 7) : Base(true) {}        // .nonpolymorphic.contract
  D(char) : Base(vf() == 7) {}                       // .polymorphic.virtual.call
  D(short) : Base(&typeid(*this) != nullptr) {}      // .polymorphic.typeid
  D(bool) : Base(dynamic_cast<D *>(this) != nullptr) {} // .polymorphic.dynamic.cast
};
D d1(1), d2(1L), d3('c'), d4((short)1), d5(true);

// ONE-COUNT-1: call void @llvm.trap()
// ONE-NOT: call void @llvm.trap()
// TWO-COUNT-2: call void @llvm.trap()
// TWO-NOT: call void @llvm.trap()
// THREE-COUNT-3: call void @llvm.trap()
// THREE-NOT: call void @llvm.trap()
// SIX-COUNT-6: call void @llvm.trap()
// SIX-NOT: call void @llvm.trap()
