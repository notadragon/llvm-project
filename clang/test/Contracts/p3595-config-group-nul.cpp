// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-config-group-nul.json -fsyntax-only %s 2>&1 | FileCheck %s

// A "group" in a P3595 configuration entry holding a NUL character is an
// error, not kept or cut at the NUL (notadragon_wg21 DECISIONS.md E192
// (1)(a)).  llvm::json decodes "\u0000" into the string, so the check sees
// it.  p3595-config-group-nul-inline.cpp is the inline form.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-group-nul.C

// CHECK: error: 'group' in entry 0 of contract configuration from {{.*}}p3595-config-group-nul.json must not contain a null character
// CHECK-NOT: error:

int f(int x) pre(x > 0) { return x; }
