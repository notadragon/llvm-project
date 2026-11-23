// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// In constant evaluation, a postcondition's result name for a class result
// constructed in place into a subobject -- a member, an array element --
// designates that subobject: o and arr are constant, and the violated o2 is
// not.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-result-name-subobject.C

struct S { int v; };
constexpr S make(const int x) post(r : r.v == x) { return S{x}; }
struct Outer { int a; S s; };
constexpr Outer o{7, make(2)}; // OK
static_assert(o.s.v == 2);
constexpr S arr[2] = {make(3), make(4)}; // OK
static_assert(arr[1].v == 4);

constexpr S make7(const int x) post(r : r.v == 7) { return S{x}; } // expected-error {{contract failed during execution of constexpr function}}
constexpr Outer o2{7, make7(5)}; // expected-error {{constexpr variable 'o2' must be initialized by a constant expression}} expected-note {{in call to 'make7(5)'}}
