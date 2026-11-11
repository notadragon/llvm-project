// RUN: not %clang_cc1 -std=c++26 -fcontracts '-fcontract-configuration=[{"match":{"group":"safety\u0000x"},"output":{"semantic":"ignore"}}]' -fsyntax-only %s 2>&1 | FileCheck %s

// A "group" in an inline P3595 configuration entry holding a NUL character
// is an error, not kept or cut at the NUL (notadragon_wg21 DECISIONS.md E192
// (1)(a)).  p3595-config-group-nul.cpp is the file form.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-group-nul-inline.C

// CHECK: error: 'group' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK-NOT: error:

int f(int x) pre(x > 0) { return x; }
