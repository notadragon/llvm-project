// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native

// A contract assertion repeated on a later declaration (the definition) and
// violated is reported, through contract_violation::location(), at the
// definition's assertion -- except for a member of a class template whose
// return type is not deduced, where it is reported at the in-class
// declaration's: for a non-template function, a member function defined
// outside its class, a member of a class template with a non-deduced and
// with a deduced return type, and a function template; preconditions and
// postconditions.  The location is implementation-defined and the
// assertions are equivalent; GCC reports the definition's in every case and
// the EDG front end the first declaration's, except for a template through
// edg-gxx (a divergence, CLANG-684 in open-issues/README.md).
// contract-repeated-location-constexpr.cpp has the same shapes in constant
// evaluation.
//
// Mirrors: gcc/testsuite/g++.dg/contracts/cpp26/contract-repeated-location.C
// in the gnu_gcc fork; in the edgcpp_compiler fork,
// tests/tests/contracts/codegen/repeated_contract_location.sft.cpp and the
// imports tests/tests/contracts/imported/{gcc,clang}/
// contract-repeated-location.sft.cpp.

#include <contracts>

static unsigned last;
static int count;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  last = v.location().line();
  ++count;
}

// Expect exactly one violation since the last check, at `expected` and not
// at `other`.
static void check(unsigned expected, unsigned other) {
  if (count != 1 || last == other || last != expected)
    __builtin_abort();
  last = 0;
  count = 0;
}

int f(const int x) pre(x > 0); enum { FPRE1 = __LINE__ };
int f(const int x) pre(x > 0) { return x; } enum { FPRE2 = __LINE__ };
int fp(const int x) post(r : r > x); enum { FPOST1 = __LINE__ };
int fp(const int x) post(r : r > x) { return x; } enum { FPOST2 = __LINE__ };

struct S {
  int m(const int x) const pre(x > 0); enum { MPRE1 = __LINE__ };
  int mp(const int x) const post(r : r > x); enum { MPOST1 = __LINE__ };
};
int S::m(const int x) const pre(x > 0) { return x; } enum { MPRE2 = __LINE__ };
int S::mp(const int x) const post(r : r > x) { return x; } enum { MPOST2 = __LINE__ };

template <class T> struct Z {
  T g(const T x) const pre(x > 0); enum { GPRE1 = __LINE__ };
  T gp(const T x) const post(r : r > x); enum { GPOST1 = __LINE__ };
  auto a(const T x) const pre(x > 0); enum { APRE1 = __LINE__ };
  auto ap(const T x) const post(r : r > x); enum { APOST1 = __LINE__ };
};
template <class T>
T Z<T>::g(const T x) const pre(x > 0) { return x; } enum { GPRE2 = __LINE__ };
template <class T>
T Z<T>::gp(const T x) const post(r : r > x) { return x; } enum { GPOST2 = __LINE__ };
template <class T>
auto Z<T>::a(const T x) const pre(x > 0) { return x; } enum { APRE2 = __LINE__ };
template <class T>
auto Z<T>::ap(const T x) const post(r : r > x) { return x; } enum { APOST2 = __LINE__ };

template <class T> T h(const T x) pre(x > 0); enum { HPRE1 = __LINE__ };
template <class T> T h(const T x) pre(x > 0) { return x; } enum { HPRE2 = __LINE__ };
template <class T> T hp(const T x) post(r : r > x); enum { HPOST1 = __LINE__ };
template <class T> T hp(const T x) post(r : r > x) { return x; } enum { HPOST2 = __LINE__ };

int main() {
  f(0);
  check(FPRE2, FPRE1);
  fp(0);
  check(FPOST2, FPOST1);

  S{}.m(0);
  check(MPRE2, S::MPRE1);
  S{}.mp(0);
  check(MPOST2, S::MPOST1);

  // A non-deduced return type: the in-class declaration's (CLANG-684).
  Z<int>{}.g(0);
  check(Z<int>::GPRE1, GPRE2);
  Z<int>{}.gp(0);
  check(Z<int>::GPOST1, GPOST2);
  Z<int>{}.a(0);
  check(APRE2, Z<int>::APRE1);
  Z<int>{}.ap(0);
  check(APOST2, Z<int>::APOST1);

  h(0);
  check(HPRE2, HPRE1);
  hp(0);
  check(HPOST2, HPOST1);
}
