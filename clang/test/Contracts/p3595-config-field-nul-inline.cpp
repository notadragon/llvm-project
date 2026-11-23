// RUN: not %clang_cc1 -std=c++26 -fcontracts \
// RUN:   '-fcontract-configuration=[{"match":{"kind":"pre\u0000x"},"output":{"semantic":"ignore"}}]' \
// RUN:   '-fcontract-configuration=[{"match":{"group":"g\u0000x"},"output":{"semantic":"ignore"}}]' \
// RUN:   '-fcontract-configuration=[{"match":{"namespace":"n\u0000x"},"output":{"semantic":"ignore"}}]' \
// RUN:   '-fcontract-configuration=[{"match":{"location":"t.C\u0000x"},"output":{"semantic":"ignore"}}]' \
// RUN:   '-fcontract-configuration=[{"match":{"caller":{"location":"t.C\u0000x"}},"output":{"semantic":"ignore"}}]' \
// RUN:   '-fcontract-configuration=[{"match":{"caller":{"namespace":"n\u0000x"}},"output":{"semantic":"ignore"}}]' \
// RUN:   '-fcontract-configuration=[{"output":{"semantic":"ignore\u0000x"}}]' \
// RUN:   '-fcontract-configuration=[{"output":{"semantic":"ignore","dynamic":{"name":"sel\u0000x"}}}]' \
// RUN:   '-fcontract-configuration=[{"output":{"semantic":"ignore","dynamic":{"name":"sel","linkage":"C\u0000x"}}}]' \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s

// A NUL character in any string field of an inline P3595 configuration
// entry is an error, not kept or cut at the NUL (notadragon_wg21
// DECISIONS.md E193 (1)): each -fcontract-configuration= above holds one in
// a different field, and each is diagnosed.  p3595-config-field-nul.cpp is
// the file form.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-field-nul-inline.C

// CHECK: error: 'kind' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'group' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'namespace' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'location' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'location' in 'caller' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'namespace' in 'caller' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'semantic' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'name' in 'dynamic' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK: error: 'linkage' in entry 0 of contract configuration from <command-line> must not contain a null character
// CHECK-NOT: error:

int f(int x) pre(x > 0) { return x; }
