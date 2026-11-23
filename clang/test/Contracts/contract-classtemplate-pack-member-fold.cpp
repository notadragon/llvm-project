// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native

// A fold over a function parameter pack whose type is a class template's
// pack, in a precondition or postcondition of a member function declared in
// the class and defined outside it -- with the contract repeated on the
// definition or not, with a deduced return type and a result name, with
// other parameter names on the definition, in a member template of the class
// template, in a member of a nested class template or of a partial
// specialization, in a member used before its definition, and in a static
// member -- is instantiated against this call's arguments.  Mirrored from
// the EDG front end, where it failed to instantiate (EDG-124 in the
// edgcpp_compiler fork).  contract-classtemplate-pack-member-fold-constexpr.cpp
// has the violations in constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-classtemplate-pack-member-fold.C
// in the gnu_gcc fork; tests/tests/contracts/templates/class_pack_member_fold.sft.cpp
// in the edgcpp_compiler fork.

#include <contracts>
#include <cstdio>

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violation_count;
}
static int failures = 0;
static void expect(int expected, const char *what) {
  if (violation_count != expected) {
    std::printf("FAIL: %s: expected %d, got %d\n", what, expected,
                violation_count);
    ++failures;
  }
  violation_count = 0;
}

template <class... T> struct Z {
  int f(const T... t) pre(((t > 0) && ...));
  auto g(const T... t) post(r : ((t > r) && ...));
  int h(const T... t) post(r : ((t + r > 0) || ...));
  int k(const T... t) pre(sizeof...(t) > 0);
  // Not repeated on the definition.
  int m(const T... t) pre((... + t) < 10);
  auto n(const T... t) post(r : (... && (r != t)));
  // Other parameter names on the definition.
  int a(const T... a) pre(((a != 7) && ...));
  template <class U> int b(U u, const T... t) pre(((t > u) && ...));
  template <class V> struct In {
    int c(V v, const T... t) pre(((t != v) && ...));
  };
};

template <class... T>
int Z<T...>::f(const T... t) pre(((t > 0) && ...)) { return 0; }
template <class... T>
auto Z<T...>::g(const T... t) post(r : ((t > r) && ...)) { return 0; }
template <class... T>
int Z<T...>::h(const T... t) post(r : ((t + r > 0) || ...)) { return -10; }
template <class... T>
int Z<T...>::k(const T... t) pre(sizeof...(t) > 0) { return 0; }
template <class... T>
int Z<T...>::m(const T... t) { return 0; }
template <class... T>
auto Z<T...>::n(const T... t) { return 2; }
template <class... T>
int Z<T...>::a(const T... b) { return 0; }
template <class... T> template <class U>
int Z<T...>::b(U u, const T... t) { return 0; }
template <class... T> template <class V>
int Z<T...>::In<V>::c(V v, const T... t) { return 0; }

// A member of a partial specialization.
template <class X, class... T> struct D;
template <class... T> struct D<int, T...> {
  int d(const T... t) pre(((t < 10) && ...));
};
template <class... T> int D<int, T...>::d(const T... t) { return 0; }

// A member odr-used before its definition.
template <class... T> struct E {
  int e(const T... t) pre(((t >= 0) && ...));
};
int use_e(int x) { return E<int, int>().e(1, x); }
template <class... T> int E<T...>::e(const T... t) { return 0; }

// A static member.
template <class... T> struct S {
  static int s(int n, const T... t) pre(((t < n) && ...));
};
template <class... T> int S<T...>::s(int n, const T... t) { return 0; }

int main() {
  Z<int, int> z;
  z.f(1, 2);
  expect(0, "f good");
  z.f(1, -2);
  expect(1, "f bad");
  z.g(1, 2);
  expect(0, "g good");
  z.g(1, -2);
  expect(1, "g bad");
  z.h(20, 2);
  expect(0, "h good");
  z.h(1, 2);
  expect(1, "h bad");
  z.k(1, 2);
  expect(0, "k");
  z.m(1, 2);
  expect(0, "m good");
  z.m(5, 6);
  expect(1, "m bad");
  z.n(1, 3);
  expect(0, "n good");
  z.n(1, 2);
  expect(1, "n bad");

  Z<long, short, int> z3;
  z3.f(1, 2, 3);
  expect(0, "f3 good");
  z3.f(1, 2, 0);
  expect(1, "f3 bad");
  z3.g(1, 2, 3);
  expect(0, "g3 good");
  z3.g(1, 0, 3);
  expect(1, "g3 bad");

  z.a(1, 2);
  expect(0, "a good");
  z.a(1, 7);
  expect(1, "a bad");
  z.b(0, 1, 2);
  expect(0, "b good");
  z.b(5, 1, 2);
  expect(1, "b bad");
  Z<int, int>::In<int>().c(3, 1, 2);
  expect(0, "c good");
  Z<int, int>::In<int>().c(1, 1, 2);
  expect(1, "c bad");
  D<int, int, long>().d(1, 2);
  expect(0, "d good");
  D<int, int, long>().d(1, 20);
  expect(1, "d bad");
  use_e(0);
  expect(0, "e good");
  use_e(-1);
  expect(1, "e bad");
  S<int, int>::s(3, 1, 2);
  expect(0, "s good");
  S<int, int>::s(2, 1, 2);
  expect(1, "s bad");

  if (failures == 0)
    std::puts("PASS");
  return failures;
}
