// RUN: %clangxx -std=c++26 -fcontracts-p4451 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4451: without -fcontracts-p3400 the assertion-control-specifier must
// name std::contracts::controls::enforced (P4451R2): another name, a P3400
// label object, a parenthesized name and a P3400 label expression are each
// one error, on every kind of contract assertion; the qualified name is
// accepted.  <contracts> declares controls::enforced (and, P4451R3,
// controls::defaulted) alone, with none of P3400's library, of types that are
// not P3400's.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-diag-other.C

#include <contracts>  // `std::contracts::controls::enforced`

struct L { using assertion_control_object = L; };
constexpr L lbl{}; // #lbl

void f(int x) pre<other>(x > 0); // expected-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}}
void g(int x) pre<std::contracts::controls::enforced>(x > 0);
void h(int x) { contract_assert<(std::contracts::controls::enforced)>(x > 0); } // expected-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}}
auto l = [](int x) pre<other>(x > 0) { return x; }; // expected-error {{name lookup finds nothing}}
void p(int x) pre<lbl>(x > 0); // expected-error {{name lookup finds another declaration}} expected-note@#lbl {{'lbl' declared here}}
void q(int x) pre<std::contracts::controls::enforced | std::contracts::controls::enforced>(x > 0); // expected-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}}
struct S {
  void m(int x) pre<other>(x > 0); // expected-error {{name lookup finds nothing}}
  void n(int x) pre<std::contracts::controls::enforced>(x > 0);
};
void k(int x) pre<std::contracts::controls::enforced>(x > 0);

auto e = std::contracts::controls::empty_label;    // expected-error {{no member named 'empty_label'}}
auto a = std::contracts::controls::always_enforce; // expected-error {{no member named 'always_enforce'}}

// The enforced label is not a P3400 assertion-control object (nor is
// defaulted: p4451-r3-defaulted.cpp).
constexpr auto &t = std::contracts::controls::enforced;
template <class T> concept control = requires { typename T::assertion_control_object; };
static_assert(!control<decltype(std::contracts::controls::enforced)>);

#ifdef __cpp_lib_contracts_labels
#error "no P3400 library"
#endif
#ifdef __cpp_contracts_labels
#error "no P3400"
#endif
