// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-samesym-ir.json -emit-llvm -o - %s | FileCheck %s

// P3595 dynamic selection: two DISTINCT config entries name the SAME emitted
// selector symbol _ZN5mylib3selEv via different spellings (C++ "mylib::sel" and
// C "_ZN5mylib3selEv"), both provideweak.  getOrCreateDynamicSelector dedups on
// the final emitted symbol, so exactly ONE weak definition is emitted.  Two
// contracts (groups "a" and "b") each match one entry, so both dispatch through
// the single shared selector.

// Exactly one definition of the selector, and it is weak.
// CHECK: define weak i16 @_ZN5mylib3selEv()
// CHECK-NOT: define {{.*}} @_ZN5mylib3selEv()

// Both contract sites call the shared selector.
// CHECK-DAG: call{{.*}} @_ZN5mylib3selEv()

void fa(int x) pre [[clang::contract_group("a")]] (x > 0) {}
void fb(int x) pre [[clang::contract_group("b")]] (x > 0) {}
