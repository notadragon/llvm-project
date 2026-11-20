// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fno-contracts -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -DPOST %s
// RUN: %clang_cc1 -std=c++26 -fno-contracts -fsyntax-only -verify -DPOST %s

// With contracts off, the extension spellings __pre and __post are not
// contract keywords, so no contract is built: the declarator ends where the
// spelling starts, and it is rejected as it would be for any other stray
// identifier there.  Each spelling gets its own RUN line because Clang's
// recovery from this error does not parse the rest of the file.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/extension-spelling-without-contracts.C

#ifndef POST
void f(int x) __pre(x > 0) {} // expected-error {{expected function body after function declarator}}
#else
void h(int x) __post(x > 0) {} // expected-error {{expected function body after function declarator}}
#endif
