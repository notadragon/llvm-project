// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4451 -fno-contracts-p4451 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4451: without -fcontracts-p4451 (or -fcontracts-p3400, which implies
// it), or with a later -fno-contracts-p4451, an assertion-control-specifier
// is rejected, and one whose id-expression is a possibly qualified
// `enforced` is pointed at the option (nothing is looked up).  Nor does
// <contracts> declare the controls namespace, and the feature-test macros
// keep P2900's values.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-diag-no-flag.C and
// p4451-ftm-no-p4451.C

#include <contracts>  // `std::contracts`

void f(int x) pre<enforced>(x > 0); // expected-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
void q(int x) pre<std::contracts::controls::enforced>(x > 0); // expected-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
void r(int x) pre<::scl::enforced>(x > 0); // expected-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
void h(int x) { contract_assert<enforced>(x > 0); } // expected-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
auto l = [](int x) pre<enforced>(x > 0) { return x; }; // expected-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
struct S {
  void m(int x) pre<enforced>(x > 0); // expected-error {{assertion-control-specifier naming 'enforced' requires '-fcontracts-p4451'}}
};

auto t = std::contracts::controls::enforced; // expected-error {{no member named 'controls'}}

#if __cpp_contracts != 202502L || __cpp_lib_contracts != 202502L
#error "P2900's values"
#endif
#ifdef __clang_contracts_p4451
#error "no P4451"
#endif
