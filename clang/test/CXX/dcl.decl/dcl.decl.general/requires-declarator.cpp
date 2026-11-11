// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// The requires-clause of an init-declarator or member-declarator belongs to
// the declarator as a whole, which must declare a templated function
// ([dcl.decl.general]); it is never part of a nested declarator or of a
// type-id.  So the clause is ill-formed exactly where no function is
// declared (Group A), and well-formed on every declarator that declares a
// function, however its return type is spelled (Group B).
//
// This file is detector D1 of the matrix: does the declaration compile at
// all.  requires-declarator-effect.cpp holds D2 (is the clause evaluated)
// and D3 (is it attached to the right function).
//
// GCC: gcc/testsuite/g++.dg/cpp2a/concepts-requires-declarator.C (GCC
// rejects Group B: upstream PR123909, PR94984).

template <class T> concept C = sizeof(T) > 0;
struct S { int m(int); };

// Group A -- no function is declared: the clause is refused.
template <class T> struct A {
  typedef int F(int) requires C<T>; // expected-error {{trailing requires clause can only be used when declaring a function}}
  using G = int(int) requires C<T>; // expected-error {{expected ';' after alias declaration}}
  int (*p)(int) requires C<T>;      // expected-error {{trailing requires clause can only be used when declaring a function}}
  void q(int bar() requires C<T>);  // expected-error {{trailing requires clause can only be used when declaring a function}}
};

// Group B -- a function is declared: the clause is well-formed.

// B1/B2: returns a pointer to function, classic spelling.
template <class T> int (*b1(T i))(int) requires C<T>;
// B3: returns a pointer to member function.
template <class T> int (S::*b3(T i))(int) requires C<T>;
// B4: trailing return type naming a pointer to function.
template <class T> auto b4(T i) -> int (*)(int) requires C<T>;
// B5: trailing return type naming a reference to function.
template <class T> auto b5(T i) -> int (&)(int) requires C<T>;
// B6: returns a pointer to array.
template <class T> char (*b6(T i))[3] requires C<T>;
// B7: returns a reference to array.
template <class T> char (&b7(T i))[3] requires C<T>;

// The same shapes as members of a class template.
template <class T> struct M {
  int (*b1(int))(int) requires C<T>;
  char (*b6(int))[3] requires C<T>;
};
template struct M<int>;

// Controls -- the shapes that work for every implementation.

// C1: the function declarator is the outermost one.
template <class T> int c1(T i) requires C<T>;
// C2: a trailing return type with no parameter list in its type-id.
template <class T> auto c2(T i) -> int requires C<T>;
// C3: B4 minus the parameter list: a trailing pointer-to-array type-id.
template <class T> auto c3(T i) -> char (*)[3] requires C<T>;
template <class T> struct N {
  int c1(int) requires C<T>;
  auto c3(int) -> char (*)[3] requires C<T>;
};
template struct N<int>;
