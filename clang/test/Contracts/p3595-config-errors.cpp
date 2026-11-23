// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration=not-json -fsyntax-only -verify %s

// P3595: an invalid inline contract configuration is diagnosed eagerly,
// through the DiagnosticsEngine, before the source file is read.  The
// diagnostic has no source location, so it is matched in any file at any
// line.

// expected-error@*:* {{invalid JSON in contract configuration}}

void f(int x) pre(x > 0) { }
