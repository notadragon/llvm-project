// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// Detectors D2 and D3 of the requires-clause declarator matrix (D1, does the
// declaration compile, is requires-declarator.cpp).
//
// D2: an unsatisfiable clause, then a call.  Honoured, the call finds no
// viable function; dropped, the call succeeds.
// D3: two overloads with complementary clauses and different return types,
// each call selecting one.  Honoured, both coexist and each initialization
// below is well-formed; a clause dropped or attached to the wrong declarator
// makes the calls ambiguous (or the second declaration a redefinition).
//
// The requires-clause after a declarator whose return type is a pointer or
// reference to function, or a pointer to array, belongs to the declared
// function ([dcl.decl.general]).
//
// GCC: gcc/testsuite/g++.dg/cpp2a/concepts-requires-declarator-effect.C (GCC
// rejects the Group B rows: upstream PR123909, PR94984).

template <class T> concept Small = sizeof(T) <= 4;
template <class T> concept Never = false; // expected-note 6 {{because 'false' evaluated to false}}

int fi(int);
long fl(int);
char arr[3];
using PFi = int (*)(int);
using PFl = long (*)(int);
using PA = char (*)[3];

// D2
template <class T> int (*n1(T))(int) requires Never<T>;        // expected-note {{constraints not satisfied}} expected-note {{does not satisfy 'Never'}}
template <class T> auto n4(T) -> int (*)(int) requires Never<T>; // expected-note {{constraints not satisfied}} expected-note {{does not satisfy 'Never'}}
template <class T> char (*n6(T))[3] requires Never<T>;         // expected-note {{constraints not satisfied}} expected-note {{does not satisfy 'Never'}}
// Controls.
template <class T> int nc1(T) requires Never<T>;                 // expected-note {{constraints not satisfied}} expected-note {{does not satisfy 'Never'}}
template <class T> auto nc2(T) -> int requires Never<T>;         // expected-note {{constraints not satisfied}} expected-note {{does not satisfy 'Never'}}
template <class T> auto nc3(T) -> char (*)[3] requires Never<T>; // expected-note {{constraints not satisfied}} expected-note {{does not satisfy 'Never'}}

void d2() {
  n1(0);  // expected-error {{no matching function for call to 'n1'}}
  n4(0);  // expected-error {{no matching function for call to 'n4'}}
  n6(0);  // expected-error {{no matching function for call to 'n6'}}
  nc1(0); // expected-error {{no matching function for call to 'nc1'}}
  nc2(0); // expected-error {{no matching function for call to 'nc2'}}
  nc3(0); // expected-error {{no matching function for call to 'nc3'}}
}

// D3
template <class T> int (*p1(T))(int) requires Small<T> { return fi; }
template <class T> long (*p1(T))(int) requires (!Small<T>) { return fl; }
PFi r1a = p1(0);
PFl r1b = p1(0LL);

template <class T> auto p4(T) -> int (*)(int) requires Small<T> { return fi; }
template <class T> auto p4(T) -> long (*)(int) requires (!Small<T>) { return fl; }
PFi r4a = p4(0);
PFl r4b = p4(0LL);

// Same return type both times: a dropped clause makes the second a
// redefinition of the first.
template <class T> char (*p6(T))[3] requires Small<T> { return &arr; }
template <class T> char (*p6(T))[3] requires (!Small<T>) { return &arr; }
PA r6a = p6(0);
PA r6b = p6(0LL);

// Controls.
template <class T> int c1(T) requires Small<T> { return 1; }
template <class T> long c1(T) requires (!Small<T>) { return 2; }
static_assert(__is_same(decltype(c1(0)), int));
static_assert(__is_same(decltype(c1(0LL)), long));

template <class T> auto c2(T) -> int requires Small<T> { return 1; }
template <class T> auto c2(T) -> long requires (!Small<T>) { return 2; }
static_assert(__is_same(decltype(c2(0)), int));
static_assert(__is_same(decltype(c2(0LL)), long));

template <class T> auto c3(T) -> char (*)[3] requires Small<T> { return &arr; }
template <class T> auto c3(T) -> char (*)[3] requires (!Small<T>) { return &arr; }
PA rc3a = c3(0);
PA rc3b = c3(0LL);
