// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=observe -triple x86_64-linux-gnu -include %t/h.h -emit-llvm -o - %t/use.cpp | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=observe -triple x86_64-linux-gnu -x c++-header %t/h.h -emit-pch -o %t/h.pch
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=observe -triple x86_64-linux-gnu -include-pch %t/h.pch -emit-llvm -o - %t/use.cpp | FileCheck %s

// Whether a predicate can throw is Sema's answer (Sema::canThrow), kept on
// the contract when it is built, and CodeGen guards the predicate for an
// evaluation_exception violation only if it can.  The answer survives a PCH:
// the same IR with the header included and with it precompiled.  `d` is the
// case CodeGen's own walk used to miss (CLANG-665): the only
// potentially-throwing part is the destructor `delete` invokes.
//
// No GCC mirror: the bit is Clang AST state with no GCC counterpart.

//--- h.h
bool may_throw(int);
bool no_throw(int) noexcept;
struct D { ~D() noexcept(false); };
inline int t(int x) pre(may_throw(x)) { return x; }
inline int n(int x) pre(no_throw(x)) { return x; }
inline int d(D *p) pre((delete p, true)) { return 0; }

//--- use.cpp
int use(D *p) { return t(1) + n(2) + d(p); }

// CHECK-LABEL: define {{.*}} @_Z1ti(
// CHECK: invoke {{.*}} @_Z9may_throwi(
// CHECK: landingpad
// CHECK: @__cxa_contract_violation_pre_observe_ex(
// CHECK-LABEL: define {{.*}} @_Z1ni(
// CHECK-NOT: landingpad
// CHECK-NOT: @__cxa_contract_violation_pre_observe_ex(
// CHECK-LABEL: define {{.*}} @_Z1dP1D(
// CHECK: invoke void @_ZN1DD1Ev(
// CHECK: @__cxa_contract_violation_pre_observe_ex(
