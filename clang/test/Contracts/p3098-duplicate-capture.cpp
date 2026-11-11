// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A name may appear only once in a P3098 capture list, as in a lambda's
// ("'y' can appear only once in a capture list"): a repeated init-capture,
// simple capture or init-capture pack is diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-duplicate-capture.C

int p7(int x) post [y = x, y = x] (true) { return 0; } // expected-error {{'y' can appear only once in a capture list}}
int p8(int x) post [x, x] (true) { return 0; } // expected-error {{'x' can appear only once in a capture list}}

template <class... T>
int p6(T... xs) post [...y = xs, ...y = xs] (true) { return 0; } // expected-error {{'y' can appear only once in a capture list}}
