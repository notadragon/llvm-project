// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A contract assertion repeated on a later declaration (the definition) and
// violated in constant evaluation is diagnosed at the definition's
// assertion -- except for a member of a class template whose return type is
// not deduced, where it is diagnosed at the in-class declaration's: the
// shapes of contract-repeated-location.cpp (a non-template function, a
// member function defined outside its class, a member of a class template
// with a non-deduced and with a deduced return type, and a function
// template; preconditions and postconditions).  The location is
// implementation-defined and the assertions are equivalent; GCC reports the
// definition's in every case and the EDG front end the first declaration's
// (a divergence, CLANG-684 in open-issues/README.md).
//
// Mirrors: gcc/testsuite/g++.dg/contracts/cpp26/contract-repeated-location-constexpr.C
// in the gnu_gcc fork; in the edgcpp_compiler fork,
// tests/tests/contracts/codegen/repeated_contract_location_constexpr.sft.cpp
// and the imports tests/tests/contracts/imported/{gcc,clang}/
// contract-repeated-location-constexpr.sft.cpp.

constexpr int f(const int x) pre(x > 0);
constexpr int f(const int x) pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int fp(const int x) post(r : r > x);
constexpr int fp(const int x) post(r : r > x) { return x; } // expected-error {{contract failed during execution of constexpr function}}

struct S {
  constexpr int m(const int x) const pre(x > 0);
  constexpr int mp(const int x) const post(r : r > x);
};
constexpr int S::m(const int x) const pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int S::mp(const int x) const post(r : r > x) { return x; } // expected-error {{contract failed during execution of constexpr function}}

template <class T> struct Z {
  // A non-deduced return type: the in-class declaration's (CLANG-684).
  constexpr T g(const T x) const pre(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  constexpr T gp(const T x) const post(r : r > x); // expected-error {{contract failed during execution of constexpr function}}
  constexpr auto a(const T x) const pre(x > 0);
  constexpr auto ap(const T x) const post(r : r > x);
};
template <class T>
constexpr T Z<T>::g(const T x) const pre(x > 0) { return x; }
template <class T>
constexpr T Z<T>::gp(const T x) const post(r : r > x) { return x; }
template <class T>
constexpr auto Z<T>::a(const T x) const pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
template <class T>
constexpr auto Z<T>::ap(const T x) const post(r : r > x) { return x; } // expected-error {{contract failed during execution of constexpr function}}

template <class T> constexpr T h(const T x) pre(x > 0);
template <class T> constexpr T h(const T x) pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
template <class T> constexpr T hp(const T x) post(r : r > x);
template <class T> constexpr T hp(const T x) post(r : r > x) { return x; } // expected-error {{contract failed during execution of constexpr function}}

// The holding calls are constant (the postconditions here never hold).
static_assert(f(1) == 1); // OK
static_assert(S{}.m(1) == 1); // OK
static_assert(Z<int>{}.g(1) == 1); // OK
static_assert(Z<int>{}.a(1) == 1); // OK
static_assert(h(1) == 1); // OK

constexpr int bad_f = f(0); // expected-error {{constexpr variable 'bad_f' must be initialized by a constant expression}} expected-note {{in call to 'f(0)'}}
constexpr int bad_fp = fp(0); // expected-error {{constexpr variable 'bad_fp' must be initialized by a constant expression}} expected-note {{in call to 'fp(0)'}}
constexpr int bad_m = S{}.m(0); // expected-error {{constexpr variable 'bad_m' must be initialized by a constant expression}} expected-note {{in call to 'S{}.m(0)'}}
constexpr int bad_mp = S{}.mp(0); // expected-error {{constexpr variable 'bad_mp' must be initialized by a constant expression}} expected-note {{in call to 'S{}.mp(0)'}}
constexpr int bad_g = Z<int>{}.g(0); // expected-error {{constexpr variable 'bad_g' must be initialized by a constant expression}} expected-note {{in call to 'Z<int>{}.g(0)'}}
constexpr int bad_gp = Z<int>{}.gp(0); // expected-error {{constexpr variable 'bad_gp' must be initialized by a constant expression}} expected-note {{in call to 'Z<int>{}.gp(0)'}}
constexpr int bad_a = Z<int>{}.a(0); // expected-error {{constexpr variable 'bad_a' must be initialized by a constant expression}} expected-note {{in call to 'Z<int>{}.a(0)'}}
constexpr int bad_ap = Z<int>{}.ap(0); // expected-error {{constexpr variable 'bad_ap' must be initialized by a constant expression}} expected-note {{in call to 'Z<int>{}.ap(0)'}}
constexpr int bad_h = h(0); // expected-error {{constexpr variable 'bad_h' must be initialized by a constant expression}} expected-note {{in call to 'h<int>(0)'}}
constexpr int bad_hp = hp(0); // expected-error {{constexpr variable 'bad_hp' must be initialized by a constant expression}} expected-note {{in call to 'hp<int>(0)'}}
