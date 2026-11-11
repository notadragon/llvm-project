// RUN: %clangxx -std=c++26 -fcontracts-p4451 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4451: without -fcontracts-p3400 the only assertion-control-specifier is
// `<terminating>`, spelled so: `terminating` is not looked up, so neither
// another name nor a qualified or parenthesized one is accepted.
// <contracts> declares labels::terminating alone, with none of P3400's
// library, of a type that is not P3400's.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-diag-other.C

#include <contracts>  // `terminating`

void f(int x) pre<other>(x > 0); // expected-error {{only '<terminating>' is allowed as an assertion-control-specifier without '-fcontracts-p3400'}}
void g(int x) pre<std::contracts::labels::terminating>(x > 0); // expected-error {{only '<terminating>' is allowed}}
void h(int x) { contract_assert<(terminating)>(x > 0); } // expected-error {{only '<terminating>' is allowed}}
auto l = [](int x) pre<other>(x > 0) { return x; }; // expected-error {{only '<terminating>' is allowed}}
struct S {
  void m(int x) pre<other>(x > 0); // expected-error {{only '<terminating>' is allowed}}
  void n(int x) pre<terminating>(x > 0);
};
void k(int x) pre<terminating>(x > 0);

auto e = std::contracts::labels::empty_label;    // expected-error {{no member named 'empty_label'}}
auto a = std::contracts::labels::always_enforce; // expected-error {{no member named 'always_enforce'}}

// The terminating label is the only declaration in the namespace, not a
// P3400 assertion-control object.
constexpr auto &t = std::contracts::labels::terminating;
template <class T> concept control = requires { typename T::assertion_control_object; };
static_assert(!control<decltype(std::contracts::labels::terminating)>);

#ifdef __cpp_lib_contracts_labels
#error "no P3400 library"
#endif
#ifdef __cpp_contracts_labels
#error "no P3400"
#endif
