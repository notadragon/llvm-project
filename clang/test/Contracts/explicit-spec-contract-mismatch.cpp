// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// An explicit specialization's redeclaration is compared with its first
// declaration like any redeclaration: a different contract, or one its first
// declaration lacks, is diagnosed.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/explicit-spec-contract-mismatch.C

template <class T> void ft(T x);
template <> void ft<int>(int x) pre(x > 5); // expected-note {{contract previously specified with a non-equivalent condition}}
template <> void ft<int>(int x) pre(x > 6) {} // expected-error {{function redeclaration differs in contract specifier sequence}} \
                                              // expected-note {{in contract specified here}}

template <class T> void gt(T x);
template <> void gt<int>(int x); // expected-note {{previously declared without contracts here}}
template <> void gt<int>(int x) pre(x > 6) {} // expected-error {{function redeclaration differs in contract specifier sequence}}
