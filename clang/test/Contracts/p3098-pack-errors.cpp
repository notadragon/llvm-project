// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s

// P3098: invalid pack-capture syntax must be diagnosed.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-pack-errors.C)
//
// ActOnPostconditionCapture rejects a pack-expansion capture whose captured
// parameter is not a pack, and a pack init-capture whose initializer contains
// no unexpanded pack.  Without both checks the declarations below are
// silently accepted.

// Pack expansion on a non-pack parameter.
void f1(int i)
  post [i...] (true); // expected-error {{pack expansion does not contain any unexpanded parameter packs}}

// Init-capture pack with no packs in the expression.
void f2(int i)
  post [...old = i] (true); // expected-error {{does not contain any unexpanded parameter packs}}
