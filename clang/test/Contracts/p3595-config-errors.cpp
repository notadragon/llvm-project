// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration=not-json -fsyntax-only %s 2>&1 | FileCheck %s

// P3595: an invalid inline contract configuration is diagnosed eagerly,
// through the DiagnosticsEngine.  Because this error aborts the invocation
// before the source file is lexed, it cannot be matched with -verify (the
// file's directives are never parsed), so it is checked with FileCheck.

// CHECK: error: invalid JSON in contract configuration

void f(int x) pre(x > 0) { }
