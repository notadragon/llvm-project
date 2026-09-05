// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// XFAIL: *

// OPEN BUG (CLANG-12 in this fork's open-issues/, GCC-27 in the gnu_gcc
// fork): two friend declarations of the same function whose contracts
// DISAGREE are accepted silently, where every other redeclaration path
// diagnoses the mismatch.
//
// Measured 2026-09-05: both compilers accept this, so both mirrors are
// xfailed. Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C
//
// On the GCC side this is a documented deferral rather than an oversight: the
// second friend declaration is discarded, and its deferred contract tokens
// with it, before end-of-class late parsing runs -- so by the time anything
// could compare them there is nothing left. The Clang side has not been
// investigated.

struct C {
  friend int f(int x) pre(x > 0);
  // expected-error@+1 {{function redeclaration differs in contract specifier sequence}}
  friend int f(int x) pre(x < 0);
};

int f(int x) { return x; }

// CONTROL: the identical mismatch at namespace scope IS diagnosed, which is
// what places the gap on the friend path rather than on contract matching.
// expected-note@+1 {{contract previously specified with a non-equivalent condition}}
int g(int x) pre(x > 0);
// expected-error@+2 {{function redeclaration differs in contract specifier sequence}}
// expected-note@+1 {{in contract specified here}}
int g(int x) pre(x < 0);
