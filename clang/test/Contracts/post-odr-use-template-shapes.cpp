// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s

// A non-const by-value parameter that is named but not odr-used in a
// postcondition -- a discarded `(void) x' operand, or a typeid operand of
// non-polymorphic type -- is fine, in a function template as in a plain
// function.  An odr-use still requires const: a plain read, and a typeid of a
// polymorphic glvalue, which is evaluated.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/post-odr-use-template-shapes.C

#include <typeinfo>

bool f1(int x) post(((void)x, true)) { return true; }
bool f2(int x) post(typeid(x) == typeid(int)) { return true; }

template <class T> bool t1(T x) post(((void)x, true)) { return true; }
template <class T> bool t2(T x) post(typeid(x) == typeid(int)) { return true; }

bool u1 = t1(1);
bool u2 = t2(1);

struct B { virtual ~B(); };
template <class T> bool t3(T x) post(x > 0) { return true; } // expected-error {{must be declared const}} expected-note {{parameter of type 'int' is declared here}}
template <class T> bool t4(T x) post(typeid(x) == typeid(B)) { return true; } // expected-error {{must be declared const}} expected-note {{parameter of type 'B' is declared here}}

bool u3 = t3(1); // expected-note {{in instantiation of}}
B b;
bool u4 = t4(b); // expected-note {{in instantiation of}}

// REQUIRES: contracts-libcxx
