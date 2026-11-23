// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -x c++-header -emit-pch -o %t/h.pch %t/h.h
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -include-pch %t/h.pch -fsyntax-only -verify %t/m.cpp

// A precompiled header's contract evaluation semantic does not override the
// translation unit's: the TU asks for enforce, so the violated precondition
// is diagnosed in constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/pch/contract-semantic-mismatch.C in the
// gnu_gcc fork.

//--- h.h
constexpr int f(int x) pre(x > 0) { return x; }

//--- m.cpp
// expected-error@*:* {{contract failed during execution of constexpr function}}
constexpr int v = f(-1); // expected-error {{constexpr variable 'v' must be initialized by a constant expression}} expected-note {{in call to 'f(-1)'}}
