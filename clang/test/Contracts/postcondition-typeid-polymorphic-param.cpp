// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// typeid of a by-value parameter of polymorphic class type in a postcondition
// odr-uses the parameter -- the operand is a glvalue of polymorphic type, so
// it is evaluated, even though the complete object's type is known -- so the
// parameter must be const, in a plain function as in a function template
// instance; a non-polymorphic operand is unevaluated, and a const or
// reference parameter is fine.  post-odr-use-template-shapes.cpp covers the
// template-only shapes.  (From the EDG fork's
// sema/postcondition_params_typeid, EDG-58.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-typeid-polymorphic-param.C

#include <typeinfo>

struct B { virtual ~B(); };
struct N {};
bool f(B a) post(typeid(a) == typeid(B)) { return true; } // expected-error {{must be declared const}} expected-note {{declared here}}
bool g(N a) post(typeid(a) == typeid(N)) { return true; } // OK, unevaluated
bool h(const B a) post(typeid(a) == typeid(B)) { return true; } // OK
bool i(B &a) post(typeid(a) == typeid(B)) { return true; } // OK, a reference
template <class T> bool t(T a) post(typeid(a) == typeid(T)) { return true; } // expected-error {{must be declared const}} expected-note {{declared here}}
template bool t<B>(B); // expected-note {{in instantiation of}}
template bool t<N>(N); // OK
