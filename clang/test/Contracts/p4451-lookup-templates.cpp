// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=p3400 -Xclang -verify-ignore-unexpected=note
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=p3400 -Xclang -verify-ignore-unexpected=note
// REQUIRES: contracts-libcxx

// P4451 [basic.contract.control]: the lookup check of an unqualified
// `enforced` in templates.  A function template's parameter, a class
// template's member declared later and a block-scope variable in a function
// template are each diagnosed once, however many specializations are
// instantiated.  Under -fcontracts-p3400 alone and both (the P3400 label),
// each program is still rejected, by P3400's label checks (a parameter or
// variable is not an assertion-control object), once per specialization
// (the instantiation notes are not checked).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-lookup-templates.C (and
// its -p3400 and -both variants)

#include <contracts>  // `std::contracts::controls::enforced`

template <class T> void f(T enforced) // #f
    pre<enforced>(true); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds another declaration}} p4451-note@#f {{declared here}} p3400-error {{value of type 'const int' is not a valid assertion-control label}} p3400-error {{value of type 'const double' is not a valid assertion-control label}}
template <class T> struct S {
  void g() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#s {{declared here}} p3400-error 2 {{value of type 'const int' is not a valid assertion-control label}}
  static constexpr int enforced = 0; // #s
};
void use() { f(1); f(2.0); S<int> s; s.g(); S<long> t; t.g(); }

template <class T>
void h(T x) {
  int enforced = 0; // #h
  contract_assert<enforced>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#h {{declared here}} p3400-error 2 {{value of type 'const int' is not a valid assertion-control label}}
}
void use2() { h(1); h(2L); }
