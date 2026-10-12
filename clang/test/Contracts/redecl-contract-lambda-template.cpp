// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A lambda that captures a parameter, in the contract of an out-of-line
// declarator with a qualified name, is captured from the declarator's
// function (the walk used to run past it and index below FunctionScopes).
// Each redeclaration below is ill-formed, no diagnostic required: within one
// translation unit the two lambda-expressions are not the same
// ([dcl.contract.func]), and Clang rejects them.
//
// Mirror: GCC rejects these ("mismatched contract condition"), as it may.

// Member of a class template, defined out of line.
template <class T> struct S { int f(int x) pre([&] { return x > 0; }()); };
// expected-note@-1 {{contract previously specified with a non-equivalent condition}}
template <class T> int S<T>::f(int x) pre([&] { return x > 0; }()) { return x; } // expected-error {{differs in contract specifier sequence}}
// expected-note@-1 {{in contract specified here}}

// Member function template.
struct A { template <class T> T g(T x) pre([&] { return x > 0; }()); };
// expected-note@-1 {{contract previously specified with a non-equivalent condition}}
template <class T> T A::g(T x) pre([&] { return x > 0; }()) { return x; } // expected-error {{differs in contract specifier sequence}}
// expected-note@-1 {{in contract specified here}}

// Non-template member, and a namespace member defined with a qualified name.
struct B { void h(int x) pre([&] { return x > 0; }()); };
// expected-note@-1 {{contract previously specified with a non-equivalent condition}}
void B::h(int x) pre([&] { return x > 0; }()) {} // expected-error {{differs in contract specifier sequence}}
// expected-note@-1 {{in contract specified here}}
namespace N { void h(int x) pre([&] { return x > 0; }()); }
// expected-note@-1 {{contract previously specified with a non-equivalent condition}}
void N::h(int x) pre([&] { return x > 0; }()) {} // expected-error {{differs in contract specifier sequence}}
// expected-note@-1 {{in contract specified here}}
