// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only -ferror-limit=0 %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// REQUIRES: contracts-libcxx

// P4451R3 [basic.contract.control]: an id-expression meant to denote
// std::contracts::controls::defaulted is held to the rule an id-expression
// for enforced is (p4451-r1-errors.cpp): each way to break it is one error
// under -fcontracts-p4451 alone.  An unqualified name lookup does not find
// (Y1) or finds as another entity (Y2), something that is not an
// id-expression (Y4), another variable, even one initialized from the label
// (Y5), a dependent name, diagnosed without instantiation (Y6), another name
// in std::contracts::controls (Y7), an ambiguous lookup (Y8), and a
// combination of the two labels (Y9).  Under -fcontracts-p3400 alone and
// both, the specifier is a P3400 label, defaulted an empty one: some of
// these are labels (Y1, Y4, Y5, Y9; a dependent label is checked at
// instantiation), the rest P3400 rejects with its own diagnostics.

#include <contracts>  // `std::contracts::controls::defaulted`

// Y1: lookup finds nothing.
void y1(int x) pre<defaulted>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}}

// Y2: lookup finds another entity: a variable, a type, and in
// complete-class context a later member, with a using-directive for the
// label in scope.
namespace y2v {
constexpr int defaulted = 0; // #y2v
void f() pre<defaulted>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#y2v {{'defaulted' declared here}} p3400-error {{value of type 'const int' is not a valid assertion-control label}}
}
namespace y2t {
struct defaulted {}; // #y2t
void f() pre<defaulted>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#y2t {{'defaulted' declared here}} p3400-error {{'defaulted' does not refer to a value}} p3400-note@#y2t {{declared here}}
}
namespace y2m {
using namespace std::contracts::controls;
struct S {
  void f() pre<defaulted>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#y2m {{'defaulted' declared here}} p3400-error {{value of type 'const int' is not a valid assertion-control label}}
  static constexpr int defaulted = 0; // #y2m
};
}

// Y4: not an id-expression.
void y4(int x) pre<(std::contracts::controls::defaulted)>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'}}

// Y5: another variable, a copy of the label.
constexpr auto t = std::contracts::controls::defaulted; // #y5
void y5(int x) pre<t>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#y5 {{'t' declared here}}

// Y6: dependent, in a function template and a class template, never
// instantiated.
template <class T> // #y6a
void y6a(T x) pre<T::defaulted>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted', not a dependent name}} p4451-note@#y6a {{'T' declared here}}
template <class T> // #y6b
struct Y6 {
  void h() { contract_assert<T::defaulted>(true); } // p4451-error {{not a dependent name}} p4451-note@#y6b {{'T' declared here}}
};
template <auto defaulted> // #y6n
void y6n() pre<defaulted>(true); // p4451-error {{not a dependent name}} p4451-note@#y6n {{'defaulted' declared here}}

// Y7: another name in std::contracts::controls, which under P4451 alone
// declares only enforced and defaulted.
void y7(int x) pre<std::contracts::controls::empty_label>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}}

// Y8: two using-directives whose namespaces are both treated as members of
// the global namespace make an unqualified `defaulted` ambiguous.
namespace y8a { constexpr int defaulted = 0; } // #y8a
void y8(int x) {
  using namespace y8a;
  using namespace std::contracts::controls;
  contract_assert<defaulted>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup is ambiguous}} p4451-note@#y8a {{'defaulted' declared here}} p4451-note@*:* {{'defaulted' declared here}} p3400-error {{reference to 'defaulted' is ambiguous}} p3400-note@#y8a {{candidate found by name lookup is 'y8a::defaulted'}} p3400-note@*:* {{candidate found by name lookup is 'std::contracts::controls::defaulted'}}
}

// Y9: the two labels combined is not an id-expression; under P3400 it is a
// combined label.
void y9(int x) pre<std::contracts::controls::defaulted | std::contracts::controls::enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'}}
