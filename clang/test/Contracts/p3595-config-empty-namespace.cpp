// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-config-empty-namespace.json -fsyntax-only %s 2>&1 | FileCheck %s

// "namespace": "" in a P3595 configuration entry is an error, in "match" and
// in "match"/"caller" alike.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-empty-namespace.C

// CHECK: error: 'namespace' in entry 0 {{.*}} must not be empty
// CHECK: error: 'namespace' in 'caller' in entry 1 {{.*}} must not be empty

int f(int x) pre(x > 0) { return x; }
