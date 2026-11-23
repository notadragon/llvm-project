// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A fold over a class template's pack in a precondition or postcondition of
// a constexpr member function declared in the class and defined outside it
// is checked in constant evaluation: the holding calls are constant, and
// each violation is diagnosed at the violated predicate (for g, the one
// repeated on the definition).  contract-classtemplate-pack-member-fold.cpp
// has the run-time checks.  Mirrored from the EDG front end (EDG-124 in the
// edgcpp_compiler fork).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-classtemplate-pack-member-fold-constexpr.C
// in the gnu_gcc fork; tests/tests/contracts/constexpr/class_pack_member_fold.sft.cpp
// in the edgcpp_compiler fork.

template <class... T> struct Z {
  constexpr int f(const T... t) const pre(((t > 0) && ...)); // expected-error {{contract failed during execution of constexpr function}}
  constexpr auto g(const T... t) const post(r : ((t > r) && ...));
};
// Other parameter names on the definition; the contract not repeated.
template <class... T>
constexpr int Z<T...>::f(const T... b) const { return 0; }
template <class... T>
constexpr auto Z<T...>::g(const T... t) const post(r : ((t > r) && ...)) { // expected-error {{contract failed during execution of constexpr function}}
  return 0;
}

constexpr Z<int, int> z;
static_assert(z.f(1, 2) == 0); // OK
static_assert(z.g(1, 2) == 0); // OK
constexpr int bad_f = z.f(1, -2); // expected-error {{constexpr variable 'bad_f' must be initialized by a constant expression}} expected-note {{in call to 'z.f(1, -2)'}}
constexpr int bad_g = z.g(1, 0);  // expected-error {{constexpr variable 'bad_g' must be initialized by a constant expression}} expected-note {{in call to 'z.g(1, 0)'}}
