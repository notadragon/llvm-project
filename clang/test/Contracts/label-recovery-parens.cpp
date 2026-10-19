// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// Without -fcontracts-p3400 an assertion-control label is reported once; a
// '>' inside parentheses does not end the label.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/label-recovery-parens.C

struct L { using assertion_control_object = L; };
constexpr L l{};
constexpr L pick(bool) { return l; }

int f(int x) pre<pick(1 > 0)>(x > 0); // expected-error {{assertion-control labels require '-fcontracts-p3400'}}
int g(int x) pre<l>(x > 0); // expected-error {{assertion-control labels require '-fcontracts-p3400'}}
