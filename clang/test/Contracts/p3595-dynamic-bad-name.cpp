// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-bad-name.json -fsyntax-only %s 2>&1 | FileCheck %s

// A P3595 "C++"-linkage selector name is a qualified name: an empty component
// other than a leading "::" is rejected when the configuration is parsed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-bad-name.C

// CHECK: error: 'name' 'a::::sel' in 'dynamic' in entry 0 {{.*}} is not a valid qualified name
// CHECK: error: 'name' 'ns::' in 'dynamic' in entry 1 {{.*}} is not a valid qualified name
// CHECK: error: 'name' '::' in 'dynamic' in entry 2 {{.*}} is not a valid qualified name

void f(int x) pre(x > 0) {}
