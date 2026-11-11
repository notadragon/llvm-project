// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=p3400
// REQUIRES: contracts-libcxx

// P4451 [basic.contract.control]: a declaration of
// std::contracts::labels::terminating shall precede each
// assertion-control-specifier.  <cassert> does not declare it; <contracts>
// does, from where it is included on.  Under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone (where the P3400 label `terminating` is not
// declared) and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-precedes.C (and its
// -p3400 and -both variants)

#include <cassert>

void f(int x) pre<terminating>(x > 0); // p4451-error {{no declaration of 'std::contracts::labels::terminating' precedes this assertion-control-specifier}} p4451-note {{declared by <contracts>}} p3400-error {{use of undeclared identifier 'terminating'}}
struct S { void m(int x) pre<terminating>(x > 0); }; // p4451-error {{no declaration of 'std::contracts::labels::terminating' precedes this assertion-control-specifier}} p4451-note {{declared by <contracts>}} p3400-error {{use of undeclared identifier 'terminating'}}
void g(int x) {
  contract_assert<terminating>(x > 0); // p4451-error {{no declaration of 'std::contracts::labels::terminating' precedes this assertion-control-specifier}} p4451-note {{declared by <contracts>}} p3400-error {{use of undeclared identifier 'terminating'}}
}

#include <contracts>  // `terminating`

void h(int x) pre<terminating>(x > 0);
struct T { void m(int x) pre<terminating>(x > 0); };
void k(int x) {
  contract_assert<terminating>(x > 0);
}
