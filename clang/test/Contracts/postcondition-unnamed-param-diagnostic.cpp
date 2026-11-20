// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// When a declaration leaves unnamed a parameter that a postcondition on
// another declaration odr-uses, and does not declare it const, the error
// names the parameter as the postcondition does (CLANG-640: it printed
// "parameter (null)"), for a later declaration of a function and for the
// definition of a function template.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-unnamed-param-redecl.C
// in the gnu_gcc fork.  Found by the EDG fork's
// sema/postcondition_params_redecl test.

void r(const int b) post(b > 0); // expected-note {{odr-used in a postcondition here}}
void r(int);                     // expected-error {{parameter 'b' referenced in contract postcondition must be declared const}}

template <class T> void t(const T b) post(b > 0); // expected-error {{parameter 'b' referenced in contract postcondition must be declared const}}
template <class T> void t(T) {}  // expected-note {{parameter of type 'int' is declared here}}
template void t<int>(int);       // expected-note {{in instantiation of function template specialization 't<int>' requested here}}
