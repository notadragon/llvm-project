// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify %s

// A postcondition result name on a function with a deduced return type: its
// type is the deduced one, and the predicate is checked in constant
// evaluation for a function, a member, a function template, a recursive
// function, a lambda and decltype(auto).  The predicate is looked up as on
// the declaration, so it sees the parameters and the enclosing scope, never
// a declaration of the same name in the body.  A deduced void return type
// with a result name is an error.  (EDG-9 shapes.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-deduced.C; the
// run-time half is Runnable/result-name-deduced.cpp.

constexpr auto f(const int x) post(r : r == x + 1) { return x + 1; }
static_assert(f(1) == 2);

struct S {
  constexpr auto m(const int x) const post(r : r == x) { return x; }
};
static_assert(S{}.m(3) == 3);

constexpr auto g(const int x) post(r : r == x) { return x + 1; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int v1 = g(1); // expected-error {{constexpr variable 'v1' must be initialized by a constant expression}} expected-note {{in call to}}

template <class T> constexpr auto t(T x) post(r : r > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int v2 = t(-1); // expected-error {{constexpr variable 'v2' must be initialized by a constant expression}} expected-note {{in call to}}

// The body's gl is not the predicate's.
constexpr int gl = 1;
constexpr auto hidden() post(r : r == gl) { const int gl = 2; return gl; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int v3 = hidden(); // expected-error {{constexpr variable 'v3' must be initialized by a constant expression}} expected-note {{in call to}}

constexpr auto rec(const int n) post(r : r == n) {
  if (n == 0)
    return 0;
  return rec(n - 1) + 1;
}
static_assert(rec(3) == 3);

constexpr auto ok = [](int x) post(r : r > 0) { return x; };
static_assert(ok(1) == 1);

constexpr int kk = 1;
constexpr auto lam = [](int x) post(r : r == kk) { const int kk = 2; return kk + x; }; // expected-error {{contract failed during execution of constexpr function}}
constexpr int v4 = lam(0); // expected-error {{constexpr variable 'v4' must be initialized by a constant expression}} expected-note {{in call to}}

int i = 0;
decltype(auto) dref(int &x) post(r : &r == &x) { return (x); }
int &ri = dref(i);

auto v() post(r : true) {} // expected-error {{result name 'r' cannot appear in a postcondition for a function with a void return type}}
