// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A result-name-introducer is an attributed-identifier followed by `:', so
// the result name may carry an attribute-specifier-seq, which applies to it
// as to any declaration.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-attributes.C

int f(int x) post(r [[maybe_unused]] : r > 0) { return x; }
int g(int x) post(r [[deprecated]] : r > 0) { return x; } // expected-warning {{'r' is deprecated}} expected-note {{'r' has been explicitly marked deprecated here}}
bool b;
int k(int x) pre(b [[maybe_unused]] ? x : 0) { return x; } // expected-error {{an attribute list cannot appear here}}
