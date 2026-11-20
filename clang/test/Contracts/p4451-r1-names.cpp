// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// P4451 [basic.contract.control] (P4451R2): the assertion-control-specifier
// is `< id-expression >`, and every way of naming the standard library
// declaration of std::contracts::controls::enforced by ordinary name lookup
// is accepted: qualified (R1), from the global namespace (R2), through a
// namespace alias (R3), unqualified after a using-directive at namespace
// scope, at block scope and in a namespace that has one (R4), after a
// using-declaration (R5), on a member function whose class has members of
// those names (R7: the specifier is a complete-class context, and lookup
// of a nested-name-specifier ignores what is not a namespace or type), and
// in templates, instantiated (R8).  R6 (`import std;`) is
// p4451-module-std.cpp.  Each compiles the same under -fcontracts-p4451
// alone, -fcontracts-p3400 alone and both: what P4451 accepts means the same
// under P3400 (E173).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-r1-spellings.C (and its
// -p3400 and -both variants)

#include <contracts>  // `std::contracts::controls::enforced`

// R1: qualified, on pre, post and contract_assert.
int r1(int x) pre<std::contracts::controls::enforced>(x > 0)
    post<std::contracts::controls::enforced>(r : r > 0) {
  contract_assert<std::contracts::controls::enforced>(x > 1);
  return x;
}

// R2: from the global namespace.
int r2(int x) pre<::std::contracts::controls::enforced>(x > 0)
    post<::std::contracts::controls::enforced>(r : r > 0) {
  contract_assert<::std::contracts::controls::enforced>(x > 1);
  return x;
}

// R3: namespace aliases, for labels and for an enclosing namespace.
namespace scl = std::contracts::controls;
namespace sc = std::contracts;
int r3(int x) pre<scl::enforced>(x > 0) post<sc::controls::enforced>(r : r > 0) {
  contract_assert<scl::enforced>(x > 1);
  return x;
}

// R4: using-directives.
namespace r4ns {
using namespace std::contracts::controls;
int f(int x) pre<enforced>(x > 0) post<enforced>(r : r > 0) {
  contract_assert<enforced>(x > 1);
  return x;
}
}
int r4q(int x) pre<r4ns::enforced>(x > 0) { return x; }
int r4b(int x) {
  using namespace std::contracts::controls;
  contract_assert<enforced>(x > 1);
  auto l = [](int y) pre<enforced>(y > 0) { return y; };
  return l(x);
}

// R5: using-declarations, at namespace and block scope.
namespace r5ns {
using std::contracts::controls::enforced;
int f(int x) pre<enforced>(x > 0) { return x; }
}
int r5b(int x) {
  using std::contracts::controls::enforced;
  contract_assert<enforced>(x > 1);
  return x;
}

// R4: a partly qualified name after a using-directive for the enclosing
// namespace.
namespace r4p {
using namespace std::contracts;
int f(int x) pre<controls::enforced>(x > 0) { return x; }
}

// R7: complete-class context.  The class has members named like each part
// of the qualified name, declared after the member function; none of them
// is a namespace, so the nested-name-specifier still finds std.  A block
// scope variable named std does not hide it either.
struct R7 {
  int f(int x) pre<std::contracts::controls::enforced>(x > 0) { return x; }
  int g(int x) pre<scl::enforced>(x > 0);
  static constexpr int std = 0;
  static constexpr int enforced = 0;
  int scl = 0;
};
int R7::g(int x) pre<scl::enforced>(x > 0) { return x; }
int r7b(int x) {
  int std = x;
  contract_assert<std::contracts::controls::enforced>(std > 0);
  return std;
}

// R8: non-dependent names in templates: a function template, a class
// template's members (defined in and out of class), a generic lambda and a
// variable template's lambda, all instantiated.
template <class T>
T r8f(T x) pre<std::contracts::controls::enforced>(x > 0)
    post<scl::enforced>(r : r > 0) {
  using namespace std::contracts::controls;
  contract_assert<enforced>(x > 1);
  return x;
}
template <class T> struct R8 {
  T m(T x) pre<scl::enforced>(x > 0) { return x; }
  T d(T x) pre<::std::contracts::controls::enforced>(x > 0);
  T a(T x) {
    contract_assert<r4ns::enforced>(x > 0);
    return x;
  }
};
template <class T> T R8<T>::d(T x) pre<::std::contracts::controls::enforced>(x > 0) { return x; }
auto r8l = [](auto x) pre<r5ns::enforced>(x > 0) { return x; };
template <class T>
auto r8v = [](T x) pre<std::contracts::controls::enforced>(x > 0) { return x; };

void use() {
  r1(2); r2(2); r3(2); r4ns::f(2); r4p::f(2); r4q(2); r4b(2); r5ns::f(2); r5b(2);
  R7 s; s.f(2); s.g(2); r7b(2);
  r8f(2); r8f(2L);
  R8<int> i; i.m(2); i.d(2); i.a(2);
  R8<long> l; l.m(2); l.d(2); l.a(2);
  r8l(2); r8l(2.0); r8v<int>(2);
}
