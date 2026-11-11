// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -fmodules -fimplicit-module-maps -fmodules-cache-path=%t/cache -I%t -fsyntax-only -verify %t/use.cpp

// Two modules that define the same inline function with different contracts
// have different definitions, diagnosed as such.  `g', whose bodies differ,
// is the control.
//
// No GCC mirror: ODRHash has no GCC counterpart.

//--- module.modulemap
module A { header "a.h" }
module B { header "b.h" }

//--- a.h
inline int f(int x) pre(x > 0) { return x; }
inline int g(int x) { return x; }

//--- b.h
inline int f(int x) pre(x > 1) { return x; }
inline int g(int x) { return x + 1; }

//--- use.cpp
#include "a.h"
#include "b.h"
int u() { return f(1) + g(1); }

// expected-error@b.h:1 {{'f' has different definitions in different modules; definition in module 'B' first difference is function contract specifiers}}
// expected-note@a.h:1 {{but in 'A' found different function contract specifiers}}
// expected-error@b.h:2 {{'g' has different definitions in different modules}}
// expected-note@a.h:2 {{but in 'A' found a different body}}
