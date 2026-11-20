// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// RUN: %{cc} -fcontracts-p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// P4451R3 [basic.contract.control]: the id-expression of an
// assertion-control-specifier may denote std::contracts::controls::defaulted
// as well as std::contracts::controls::enforced, found by the same ordinary
// name lookup: qualified (D1), from the global namespace (D2), through a
// namespace alias (D3), unqualified after a using-directive at namespace
// scope, at block scope and in a namespace that has one (D4), after a
// using-declaration (D5), on a member function whose class has members of
// those names (D7), and in templates, instantiated (D8).  Both labels may be
// used on one function (D9).  Under -fcontracts-p4451 alone <contracts>
// declares defaulted of an internal type that is no P3400 assertion-control
// object; under -fcontracts-p3400 it is an empty label, of the type of
// empty_label, with no facet (D10).  Each compiles the same under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone and both.

#include <contracts>  // `std::contracts::controls::defaulted`

// D1: qualified, on pre, post and contract_assert.
int d1(int x) pre<std::contracts::controls::defaulted>(x > 0)
    post<std::contracts::controls::defaulted>(r : r > 0) {
  contract_assert<std::contracts::controls::defaulted>(x > 1);
  return x;
}

// D2: from the global namespace.
int d2(int x) pre<::std::contracts::controls::defaulted>(x > 0)
    post<::std::contracts::controls::defaulted>(r : r > 0) {
  contract_assert<::std::contracts::controls::defaulted>(x > 1);
  return x;
}

// D3: namespace aliases, for the labels and for an enclosing namespace.
namespace scl = std::contracts::controls;
namespace sc = std::contracts;
int d3(int x) pre<scl::defaulted>(x > 0)
    post<sc::controls::defaulted>(r : r > 0) {
  contract_assert<scl::defaulted>(x > 1);
  return x;
}

// D4: using-directives.
namespace d4ns {
using namespace std::contracts::controls;
int f(int x) pre<defaulted>(x > 0) post<defaulted>(r : r > 0) {
  contract_assert<defaulted>(x > 1);
  return x;
}
}
int d4q(int x) pre<d4ns::defaulted>(x > 0) { return x; }
int d4b(int x) {
  using namespace std::contracts::controls;
  contract_assert<defaulted>(x > 1);
  auto l = [](int y) pre<defaulted>(y > 0) { return y; };
  return l(x);
}
namespace d4p {
using namespace std::contracts;
int f(int x) pre<controls::defaulted>(x > 0) { return x; }
}

// D5: using-declarations, at namespace and block scope.
namespace d5ns {
using std::contracts::controls::defaulted;
int f(int x) pre<defaulted>(x > 0) { return x; }
}
int d5b(int x) {
  using std::contracts::controls::defaulted;
  contract_assert<defaulted>(x > 1);
  return x;
}

// D7: complete-class context.  The class has members named like each part
// of the qualified name, declared after the member function; none of them
// is a namespace, so the nested-name-specifier still finds std.
struct D7 {
  int f(int x) pre<std::contracts::controls::defaulted>(x > 0) { return x; }
  int g(int x) pre<scl::defaulted>(x > 0);
  static constexpr int std = 0;
  static constexpr int defaulted = 0;
  int scl = 0;
};
int D7::g(int x) pre<scl::defaulted>(x > 0) { return x; }

// D8: non-dependent names in templates, instantiated.
template <class T>
T d8f(T x) pre<std::contracts::controls::defaulted>(x > 0)
    post<scl::defaulted>(r : r > 0) {
  using namespace std::contracts::controls;
  contract_assert<defaulted>(x > 1);
  return x;
}
template <class T> struct D8 {
  T m(T x) pre<scl::defaulted>(x > 0) { return x; }
  T d(T x) pre<::std::contracts::controls::defaulted>(x > 0);
  T a(T x) {
    contract_assert<d4ns::defaulted>(x > 0);
    return x;
  }
};
template <class T>
T D8<T>::d(T x) pre<::std::contracts::controls::defaulted>(x > 0) { return x; }
auto d8l = [](auto x) pre<d5ns::defaulted>(x > 0) { return x; };

// D9: both labels, and unlabeled assertions, on one function.
int d9(int x) pre<scl::defaulted>(x > 0) pre<scl::enforced>(x > 1) pre(x > 2)
    post<scl::enforced>(r : r > 0) post<scl::defaulted>(r : r > 1) {
  contract_assert<scl::defaulted>(x > 3);
  contract_assert<scl::enforced>(x > 4);
  return x;
}

// D10: the library declarations.
static_assert(!__is_same(decltype(scl::defaulted), decltype(scl::enforced)));
#ifdef __cpp_lib_contracts_labels
static_assert(__is_same(decltype(scl::defaulted), const scl::empty_label_t));
static_assert(scl::is_assertion_control_object<scl::defaulted>);
static_assert(!scl::is_semantic_computation_label<scl::defaulted>);
static_assert(!scl::is_allowed_semantics_label<scl::defaulted>);
static_assert(!scl::is_local_violation_label<scl::defaulted>);
static_assert(!scl::is_queryable_label<scl::defaulted>);
static_assert(!scl::is_identification_label<scl::defaulted>);
static_assert(!scl::is_compute_comment_label<scl::defaulted>);
static_assert(!scl::is_compute_message_label<scl::defaulted>);
#else
template <class T>
concept control = requires { typename T::assertion_control_object; };
static_assert(!control<decltype(scl::defaulted)>);
#endif

void use() {
  d1(2); d2(2); d3(2); d4ns::f(2); d4p::f(2); d4q(2); d4b(2); d5ns::f(2);
  d5b(2);
  D7 s; s.f(2); s.g(2);
  d8f(2); d8f(2L);
  D8<int> i; i.m(2); i.d(2); i.a(2);
  D8<long> l; l.m(2); l.d(2); l.a(2);
  d8l(2); d8l(2.0);
  d9(5);
}
