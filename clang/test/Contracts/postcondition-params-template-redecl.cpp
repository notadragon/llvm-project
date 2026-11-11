// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A parameter a postcondition odr-uses must be const on every declaration
// ([dcl.contract.func]), so a function template's redeclaration without the
// postcondition, whose parameter type is dependent, is checked in each
// instance: an error where the instance's parameter is a non-const object,
// and fine where it is a reference (exempt) or const from the template
// argument.  A friend naming a member of a class template is not a
// redeclaration of a function template.  (From the EDG fork's
// sema/postcondition_params_template_redecl, EDG-57.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-postcondition-redecl-reference-param.C
// (GCC also rejects the reference instance, a GCC defect).

template <typename T> void f(T const a) post(a); // expected-error 2 {{parameter 'a' referenced in contract postcondition must be declared const}}
template <typename T> void f(T a);               // expected-note {{parameter of type 'int' is declared here}} expected-note {{parameter of type 'long' is declared here}}
template <typename T> void f(T const a) {}
template void f<int>(int);   // expected-note {{in instantiation of}}
template void f<long>(long); // expected-note {{in instantiation of}}

template <typename T> void g(T const a) post(a);
template <typename T> void g(T a);               // OK, a reference in g<int &>
template <typename T> void g(T const a) {}
template void g<int &>(int &);

template <typename T> void h(T const a) post(a);
template <typename T> void h(T a);               // OK, const in h<const int>
template <typename T> void h(T const a) {}
template void h<const int>(const int);

template <typename T> int k(T const a) post(a > 0); // expected-error {{parameter 'a' referenced in contract postcondition must be declared const}}
template <typename U> int k(U b);                   // expected-note {{parameter of type 'int' is declared here}}
template <typename T> int k(T const a) { return a; }
int use() { return k(1); } // expected-note {{in instantiation of}}

template <typename T> struct A { int m(int); int n(const int a) post(a > 0); };
class C {
  template <typename T> friend int A<T>::m(int);
  template <typename T> friend int A<T>::n(const int a);
};
