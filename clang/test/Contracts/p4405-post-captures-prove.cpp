// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fcxx-exceptions -fexceptions -emit-llvm-only -verify %s

// P4405: a postcondition with captures (P3098) under prove is an error and
// nothing about its captures is emitted, so there is no internal error.  A
// precondition is analyzed at each call (D4405).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-post-captures-prove.C

int cap(int x);
int f(int x) post [c = cap(x)] (c > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int g(int x) pre(x > 0) post [c = cap(x)] (r: r > c) { return x; }
// expected-error@-1 {{contract assertion cannot be proven}}
// expected-note@-2 {{the predicate of the contract assertion}}

int use(int y) { return g(y); } // expected-error {{contract assertion cannot be proven}}
