// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A non-reference parameter odr-used in a postcondition must be const on
// every declaration of the function ([dcl.contract.func]), including an
// out-of-line definition of a class-template member function taking `T x'.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-post-param-const.C

template <class T> struct S { void m(const T x) post(x > 0); }; // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}}
template <class T> void S<T>::m(T x) {} // expected-note {{parameter of type 'int' is declared here}}
void u() { S<int>{}.m(1); } // expected-note {{in instantiation of member function 'S<int>::m' requested here}}
