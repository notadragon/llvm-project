// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-noweak-ir.json -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-noweak-ir.json -emit-llvm -o - %s | FileCheck --check-prefix=CALL %s

// P3595 dynamic selection with provideweak:false: the compiler must NOT emit a
// weak definition of the selector -- the user is responsible for supplying a
// strong definition at link time.  The Runnable test p3595-dynamic-noweak.cpp
// only observes link+run success, which a stray compiler-emitted weak def would
// silently mask (the user's strong def would win).  This test observes the IR
// directly: the selector is called, but never defined by the compiler.
//
// (With provideweak:true the compiler DOES emit `define weak i8
// @_Z16p3595_noweak_selv`, so this CHECK-NOT is non-vacuous.)

namespace std::contracts {
  enum class evaluation_semantic : unsigned char {
    ignore = 1, observe, enforce, quick_enforce
  };
}

std::contracts::evaluation_semantic p3595_noweak_sel();

void f(int x) pre(x > 0) { }

// The selector is called ...
// CALL: call{{.*}} @_Z16p3595_noweak_selv
// ... but the compiler must not define it anywhere (weak or otherwise).
// CHECK-NOT: define{{.*}} @_Z16p3595_noweak_selv
