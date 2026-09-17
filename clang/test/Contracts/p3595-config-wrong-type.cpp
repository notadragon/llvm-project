// RUN: not %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/p3595-config-wrong-type.json \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s

// P3595 x config: a known key present with the wrong JSON type is diagnosed and
// its entry skipped, rather than being silently ignored.
//
// This matters more than it looks.  Dropping a *match* criterion makes an entry
// LESS selective, so a mistyped "kind" would quietly turn a narrowly-scoped
// entry into a catch-all that changes the semantic of every contract in the
// translation unit -- with no diagnostic at all.
//
// GCC diagnoses each of these ("%<kind%> must be a string" and friends, see
// gcc/c-family/contracts-config.cc); this keeps the two implementations
// aligned.  Configuration is validated eagerly, so these fire even though this
// translation unit contains no contract assertions.

// CHECK: error: 'kind' in entry 0 of contract configuration from {{.*}} must be a string
// CHECK: error: 'group' in entry 1 of contract configuration from {{.*}} must be a string
// CHECK: error: 'namespace' in entry 2 of contract configuration from {{.*}} must be a string
// CHECK: error: 'location' in entry 3 of contract configuration from {{.*}} must be a string
// CHECK: error: 'constexpr' in entry 4 of contract configuration from {{.*}} must be a boolean
// CHECK: error: 'location' in entry 5 of contract configuration from {{.*}} must be a string
// CHECK: error: 'namespace' in entry 6 of contract configuration from {{.*}} must be a string
// CHECK: error: 'semantic' in entry 7 of contract configuration from {{.*}} must be a string
// CHECK: error: 'name' in entry 8 of contract configuration from {{.*}} must be a string
// CHECK: error: 'linkage' in entry 9 of contract configuration from {{.*}} must be a string
// CHECK: error: 'provideweak' in entry 10 of contract configuration from {{.*}} must be a boolean

int f(int x) { return x; }
