// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// GCC-938's case: a postcondition diagnosed as odr-using a value parameter
// that is not const is erroneous, and is not compared with a
// redeclaration's.  (Clang diagnoses only the first declaration.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-nonconst-param-redecl.C

int g(int x)                    // expected-note {{parameter of type 'int' is declared here}}
  post(r: r > x);               // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}}

int g(const int x)
  post(r: r > x)
{
  return x + 1;
}
