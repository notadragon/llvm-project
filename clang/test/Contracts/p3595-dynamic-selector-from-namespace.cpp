// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-selector-from-namespace.json -emit-llvm -o - %s | FileCheck %s --implicit-check-not=_ZN2n13lib --implicit-check-not=_ZN2n22n33lib --implicit-check-not=_ZN2n43lib

// P3595: a "C++"-linkage selector name is qualified from the global
// namespace, wherever the checked function is: a namespace, nested
// namespaces, or a member function defined in its class.  No selector is
// declared, so every check calls the one synthesized lib::detail::sel().
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-selector-from-namespace.C
// in the gnu_gcc fork (GCC-655), whose CLANG-647 row asked for it.

namespace n1 { void f(int x) pre(x > 0) {} }
namespace n2 { namespace n3 { void g(int x) pre(x > 0) {} } }
namespace n4 { struct S { void h(int x) pre(x > 0) {} }; }
void use(n4::S s) { s.h(1); }

// CHECK-DAG: define {{.*}}void @_ZN2n11fEi(
// CHECK-DAG: define {{.*}}void @_ZN2n22n31gEi(
// CHECK-DAG: define {{.*}}void @_ZN2n41S1hEi(
// CHECK-DAG: define weak {{.*}} @_ZN3lib6detail3selEv()
// CHECK-DAG: call {{.*}} @_ZN3lib6detail3selEv()
