// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-constexpr-ub-diagnostic.json -fsyntax-only -verify %s

// Undefined behaviour reached during constant evaluation while the P3100
// implicit checks have a checking semantic is diagnosed by naming the UB.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-constexpr-ub-diagnostic.C

constexpr int d(int x, int y) { return x / y; } // expected-note {{division by zero}}
constexpr int s(int x, int y) { return x << y; } // expected-note {{shift count 99 >= width of type 'int'}}
constexpr int fc(double v) { return (int)v; } // expected-note {{value 1.0E+30 is outside the range of representable values of type 'int'}}
constexpr int g(int x) { [[assume(x > 0)]]; return x; } // expected-note {{assumption evaluated to false}}

constexpr int v1 = d(1, 0); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'd(1, 0)'}}
constexpr int v2 = s(1, 99); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 's(1, 99)'}}
constexpr int v3 = fc(1e30); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'fc(1.000000e+30)'}}
constexpr int v4 = g(-1); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'g(-1)'}}
