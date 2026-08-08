// P3595: output.dynamic parses cleanly (no diagnostics).
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-parse.json -fsyntax-only -verify %s
// expected-no-diagnostics
void f(int x) pre(x > 0) { }
