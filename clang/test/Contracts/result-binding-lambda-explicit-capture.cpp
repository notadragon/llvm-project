// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A result binding is a local entity ([basic.pre]), so it can be named in a
// lambda's capture list; the standard's [expr.prim.id.unqual] example copies
// it with `[=] mutable { ++r; ... }'.  The closure member of a by-copy capture
// is not const, so ++r in the mutable lambda is fine.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-binding-lambda-ref-constify.C
// for b0 (GCC also rejects b1, not covered there).

int b0() post(r: [r] { return r; }() > 0) { return 1; }
int b1() post(r: [r]() mutable { return ++r; }() > 0) { return 1; }
int b2() post(r: [&r] { return r; }() > 0) { return 1; }
