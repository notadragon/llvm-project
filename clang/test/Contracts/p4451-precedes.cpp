// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=p3400
// REQUIRES: contracts-libcxx

// P4451 [basic.contract.control] (P4451R2): name lookup for the
// assertion-control-specifier's id-expression shall find the standard library
// declaration of std::contracts::controls::enforced, so that declaration
// must be visible.  <cassert> does not declare it; <contracts> does, from
// where it is included on.  An unqualified `enforced` that lookup does
// not find is an error before and after <contracts> (R0 accepted it after);
// a qualified name only after.  Under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone (where the P3400 label `enforced` is not
// declared before <contracts>, and the contract_control directive finds it
// after) and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-precedes.C (and its
// -p3400 and -both variants)

#include <cassert>

void f(int x) pre<enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{use of undeclared identifier 'enforced'}}
struct S { void m(int x) pre<enforced>(x > 0); }; // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{use of undeclared identifier 'enforced'}}
void g(int x) {
  contract_assert<enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error {{use of undeclared identifier 'enforced'}}
}
void q(int x) pre<std::contracts::controls::enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error-re {{{{use of undeclared identifier 'std'|no member named}}}}
struct R { void m(int x) pre<::std::contracts::controls::enforced>(x > 0); }; // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p4451-note {{'<contracts>' declares them}} p3400-error-re {{{{use of undeclared identifier 'std'|no member named}}}}

#include <contracts>  // `std::contracts::controls::enforced`

void u(int x) pre<enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}}
void h(int x) pre<std::contracts::controls::enforced>(x > 0);
struct T { void m(int x) pre<::std::contracts::controls::enforced>(x > 0); };
void k(int x) {
  using namespace std::contracts::controls;
  contract_assert<enforced>(x > 0);
}
