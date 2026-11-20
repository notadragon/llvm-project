// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4451 -fno-contracts-p4451 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4451R3: without -fcontracts-p4451 (or -fcontracts-p3400), or with a
// later -fno-contracts-p4451, an assertion-control-specifier whose
// id-expression is a possibly qualified `defaulted` is pointed at the option
// as one naming `enforced` is (nothing is looked up), on every kind of
// assertion, a member function's late-parsed one included.  <contracts>
// does not declare the label.  Any other specifier is diagnosed as a P3400
// label.

#include <contracts>  // `std::contracts`

void f(int x) pre<defaulted>(x > 0); // expected-error {{assertion-control-specifier naming 'defaulted' requires '-fcontracts-p4451'}}
void q(int x) pre<std::contracts::controls::defaulted>(x > 0); // expected-error {{assertion-control-specifier naming 'defaulted' requires '-fcontracts-p4451'}}
void r(int x) pre<::scl::defaulted>(x > 0); // expected-error {{assertion-control-specifier naming 'defaulted' requires '-fcontracts-p4451'}}
void h(int x) { contract_assert<defaulted>(x > 0); } // expected-error {{assertion-control-specifier naming 'defaulted' requires '-fcontracts-p4451'}}
auto l = [](int x) pre<defaulted>(x > 0) { return x; }; // expected-error {{assertion-control-specifier naming 'defaulted' requires '-fcontracts-p4451'}}
struct S {
  void m(int x) pre<defaulted>(x > 0); // expected-error {{assertion-control-specifier naming 'defaulted' requires '-fcontracts-p4451'}}
};

auto t = std::contracts::controls::defaulted; // expected-error {{no member named 'controls'}}

