// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-caller-object-parse-good.json -fsyntax-only %s 2>&1 | FileCheck --check-prefix=GOOD %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-caller-object-parse-bad.json -fsyntax-only %s 2>&1 | FileCheck --check-prefix=BAD %s

// P3595: test parsing of the "caller" object form (as opposed to the
// boolean-only form) in contract configuration "match" entries.
//
// Good config: "caller" is an object with "location"/"namespace" (both
// parsed) plus an unknown "module" key (must warn).
// Bad config: "caller" is neither a boolean nor an object (must error with
// a clear diagnostic instead of being silently dropped).
//
// The bad config's error aborts the invocation before the file is lexed, so
// these are checked with FileCheck rather than -verify.

// GOOD: warning: unknown key 'module' in 'caller' object in contract configuration from{{.*}}p3595-caller-object-parse-good.json

// BAD: error: 'caller' in entry 0 of contract configuration from{{.*}}p3595-caller-object-parse-bad.json must be a boolean or object

void f(int x) pre(x > 0) { }
