// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify=enforce %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify=observe %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -triple x86_64-unknown-linux-gnu -emit-llvm -o %t.ll %s -DCODEGEN
// RUN: FileCheck %s < %t.ll
// RUN: FileCheck %s --check-prefix=NODT < %t.ll

// A variable's constant destruction evaluates the destructor's contract
// assertions as any other constant evaluation does (CLANG-862: they were
// ignored).  A constexpr variable, which must have constant destruction
// ([dcl.constexpr]), and a class-type template argument are ill-formed when
// a precondition is violated or is not a core constant expression under a
// terminating semantic, and get a warning under observe.  The destruction
// of any other variable is evaluated only to learn whether the object needs
// destroying at exit: a contract problem makes it non-constant, so the
// destructor is registered and its precondition evaluated at run time.
// The same destructor run inside a constant evaluation is
// constexpr-dtor-contracts.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-cdtor-contracts.C

struct DF {
  constexpr DF() {}
  constexpr ~DF() pre(false) {} // enforce-error 2 {{contract failed during execution of constexpr function}} observe-warning 2 {{contract failed during execution of constexpr function}}
};

int g; // enforce-note {{declared here}}
struct DE {
  constexpr ~DE() pre(g > 0) {} // enforce-error {{contract failed during execution of constexpr function}} observe-warning {{contract failed during execution of constexpr function}} enforce-note {{read of non-const variable 'g' is not allowed in a constant expression}}
};

struct DT {
  constexpr ~DT() pre(true) {}
};

#ifndef CODEGEN
constexpr DF f2; // enforce-error {{constexpr variable 'f2' must have constant destruction}} enforce-note 0+ {{}}

template <DF x> struct X {};
X<DF{}> xv; // enforce-error {{non-type template argument is not a constant expression}} enforce-note 0+ {{}}

constexpr DE e2; // enforce-error {{constexpr variable 'e2' must have constant destruction}} enforce-note 0+ {{}}
#endif

// Not constant destruction: destroyed at exit, through the destructor.
// CHECK-DAG: call i32 @__cxa_atexit(ptr @_ZN2DED{{[12]}}Ev, ptr @e1,
DE e1;
// CHECK-DAG: call i32 @__cxa_atexit(ptr @_ZN2DED{{[12]}}Ev, ptr @_ZZ1fvE2el,
void f() { static DE el; }
// CHECK-DAG: call i32 @__cxa_atexit(ptr @_ZN2DFD{{[12]}}Ev, ptr @ff,
DF ff;

// A destructor whose contract assertions hold still has constant
// destruction: never destroyed.
// NODT-NOT: @_ZN2DTD
DT t1;
constexpr DT t2;
