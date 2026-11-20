// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-config-field-nul.json -fsyntax-only %s 2>&1 | FileCheck %s

// A NUL character in any string field of a P3595 configuration entry is an
// error, not kept or cut at the NUL (notadragon_wg21 DECISIONS.md E193 (1)):
// each entry of p3595-config-field-nul.json holds one in a different field,
// and each is diagnosed and skipped.  p3595-config-field-nul-inline.cpp is
// the inline form; p3595-config-group-nul.cpp is the "group" field alone.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-field-nul.C

// CHECK: error: 'kind' in entry 0 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'group' in entry 1 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'namespace' in entry 2 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'location' in entry 3 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'location' in 'caller' in entry 4 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'namespace' in 'caller' in entry 5 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'semantic' in entry 6 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'name' in 'dynamic' in entry 7 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK: error: 'linkage' in entry 8 of contract configuration from {{.*}}p3595-config-field-nul.json must not contain a null character
// CHECK-NOT: error:

int f(int x) pre(x > 0) { return x; }
