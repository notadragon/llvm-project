// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -triple x86_64-unknown-linux-gnu -O2 -disable-llvm-passes -mconstructor-aliases -emit-llvm -o - %s | FileCheck %s
// REQUIRES: x86-registered-target

// A destructor whose body is empty, whose members need no destruction and
// whose one base with a nontrivial destructor is at offset 0 is emitted, with
// constructor aliases on (the driver's default on most targets) and
// optimization, as an alias of that base's base-object destructor -- which
// dropped the destructor's own contract assertions, evaluated in its body
// (CLANG-869).  A destructor with contract assertions is emitted as a
// function of its own, virtual or not, whichever declaration has them.
// (Clang's own test: gnu_gcc never makes a destructor an alias of its
// base's.)

int g;

// Inline: the uses were replaced by the base's destructor.
struct B { virtual ~B(); };
struct D : B { ~D() override pre(g > 0) post(g == 1) {} };

// Defined out of line, over a base destructor defined here: an alias.
struct NB { ~NB(); };
NB::~NB() {}
struct ND : NB { ~ND() post(g == 1); };
ND::~ND() {}

void use() { D d; ND nd; }

// CHECK-NOT: @_ZN2NDD2Ev = {{.*}}alias
// CHECK-DAG: define {{.*}}void @_ZN1DD2Ev(
// CHECK-DAG: define {{.*}}void @_ZN2NDD2Ev(
