// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only -ferror-limit=0 %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=p3400
// REQUIRES: contracts-libcxx

// P4451 [basic.contract.control] (P4451R2): the id-expression of an
// assertion-control-specifier shall not be type-dependent or value-dependent,
// and name lookup for it shall find the standard library declaration of
// std::contracts::controls::enforced.  Each way to break that is one error
// under -fcontracts-p4451 alone (the shared R1 list X1-X8; X3, a qualified
// name before <contracts>, is in p4451-precedes.cpp): an unqualified name
// lookup does not find (X1) or finds as another entity (X2), something that
// is not an id-expression (X4), the name of another variable, even one
// initialized from the label (X5), a dependent name, diagnosed without
// instantiation (X6), another name in std::contracts::controls (X7), and an
// ambiguous lookup (X8).  Under -fcontracts-p3400 alone and both, the
// specifier is a P3400 label: some of these are labels (X1, X4, X5, a
// dependent label is checked at instantiation), the rest P3400 rejects with
// its own diagnostics.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-r1-edges.C (and its
// -p3400 and -both variants)

#include <contracts>  // `std::contracts::controls::enforced`

// X1: lookup finds nothing (R0 accepted this).
void x1(int x) pre<enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}}

// X2: lookup finds another entity: a variable, a function, a type, and in
// complete-class context a later member, with a using-directive for the
// label in scope; and a namespace alias a later member type hides.
namespace x2v {
constexpr int enforced = 0; // #x2v
void f() pre<enforced>(true); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds another declaration}} p4451-note@#x2v {{'enforced' declared here}} p3400-error {{value of type 'const int' is not a valid assertion-control label}}
}
namespace x2f {
void enforced(); // #x2f
void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#x2f {{'enforced' declared here}} p3400-error {{value of type 'void ()' is not a valid assertion-control label}}
}
namespace x2t {
struct enforced {}; // #x2t
void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#x2t {{'enforced' declared here}} p3400-error {{'enforced' does not refer to a value}} p3400-note@#x2t {{declared here}}
}
namespace x2m {
using namespace std::contracts::controls;
struct S {
  void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#x2m {{'enforced' declared here}} p3400-error {{value of type 'const int' is not a valid assertion-control label}}
  static constexpr int enforced = 0; // #x2m
};
}
namespace x2a {
namespace scl = std::contracts::controls;
struct S {
  void f() pre<scl::enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#x2a {{'scl' declared here}} p3400-error {{'scl' (aka 'int') is not a class, namespace, or enumeration}}
  using scl = int; // #x2a
};
}

// X4: not an id-expression.
void x4(int x) pre<(std::contracts::controls::enforced)>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}}
void x4e(int x) pre<>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}} p3400-error {{expected expression}}

// X5: another variable, and a static data member, each a copy of the label.
constexpr auto t = std::contracts::controls::enforced; // #x5
void x5(int x) pre<t>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#x5 {{'t' declared here}}
struct Q { // #x5q
  static constexpr auto enforced = std::contracts::controls::enforced;
};
void x5q(int x) pre<Q::enforced>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#x5q {{'Q' declared here}}

// X6: dependent, in a function template and a class template, never
// instantiated.
template <class T> // #x6a
void x6a(T x) pre<T::enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted', not a dependent name}} p4451-note@#x6a {{'T' declared here}}
template <class T> // #x6b
struct X6 {
  void g() pre<T::N::enforced>(true); // p4451-error {{not a dependent name}} p4451-note@#x6b {{'T' declared here}}
  void h() { contract_assert<T::enforced>(true); } // p4451-error {{not a dependent name}} p4451-note@#x6b {{'T' declared here}}
};
template <auto enforced> // #x6n
void x6n() pre<enforced>(true); // p4451-error {{not a dependent name}} p4451-note@#x6n {{'enforced' declared here}}

// X7: another name in std::contracts::controls, which under P4451 alone
// declares only enforced and defaulted.
void x7(int x) pre<std::contracts::controls::enforce>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}} p3400-error {{no member named 'enforce' in namespace 'std::contracts::controls'; did you mean 'enforced'?}} p3400-note@*:* {{'enforced' declared here}}

// X8: two using-directives whose namespaces are both treated as members of
// the global namespace make an unqualified `enforced` ambiguous.
namespace x8a { constexpr int enforced = 0; } // #x8a
void x8(int x) {
  using namespace x8a;
  using namespace std::contracts::controls;
  contract_assert<enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup is ambiguous}} p4451-note@#x8a {{'enforced' declared here}} p4451-note@*:* {{'enforced' declared here}} p3400-error {{reference to 'enforced' is ambiguous}} p3400-note@#x8a {{candidate found by name lookup is 'x8a::enforced'}} p3400-note@*:* {{candidate found by name lookup is 'std::contracts::controls::enforced'}}
}

// Mirrors of R1-GCC's further cases.  Unqualified in the global namespace
// only (P3400's contract_control directive there finds the label).
void g1(int x) pre<::enforced>(x > 0); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds nothing}}
// A template-id in the qualifier, including a `>>` closing two argument
// lists; a class's static reference member bound to the label; a private
// member, with no access diagnostic besides.
template <class T> struct W {
  static constexpr auto enforced = std::contracts::controls::enforced;
};
void g2(int x) pre<W<int>::enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}}
void g2b(int x) pre<W<W<int>>::enforced>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}}
struct C { // #g3
  static constexpr const auto &enforced = std::contracts::controls::enforced;
};
void g3(int x) pre<C::enforced>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#g3 {{'C' declared here}}
class P {
  static constexpr int enforced = 0; // #g3p
  void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#g3p {{'enforced' declared here}} p3400-error {{value of type 'const int' is not a valid assertion-control label}}
};
// Expressions that are not an id-expression.
void g4(int x) pre<std::contracts::controls::enforced + 0>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}} p3400-error {{invalid operands to binary expression}}
void g5(int x) pre<std::contracts::controls::enforced()>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}} p3400-error {{does not provide a call operator}}
void g6(int x) pre<1>(x > 0); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}} p3400-error {{value of type 'int' is not a valid assertion-control label}}
void g7(int x) { contract_assert<>(x > 0); } // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}} p3400-error {{expected expression}}
// A block-scope variable hides the using-directive.
namespace g8 {
using namespace std::contracts::controls;
void f(int x) {
  int enforced = x; // #g8
  contract_assert<enforced>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#g8 {{'enforced' declared here}} p3400-error {{value of type 'int' is not a valid assertion-control label}}
}
}
// Dependent, each diagnosed once at definition though the instantiation
// would name the label: through a member alias of a template parameter,
// with `typename`, and a reference template parameter.
struct Lab {
  static constexpr const auto &enforced = std::contracts::controls::enforced;
};
template <class T> struct G9 {
  using N = T; // #g9
  void f() pre<N::enforced>(true); // p4451-error {{not a dependent name}} p4451-note@#g9 {{'N' declared here}}
};
template <class T>
void g10() pre<typename T::N::enforced>(true); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced'}} p3400-error {{expected '(' for function-style cast or type construction}}
template <auto &enforced> // #g11
void g11() pre<enforced>(true); // p4451-error {{not a dependent name}} p4451-note@#g11 {{'enforced' declared here}}
void use_g() { G9<Lab> d; d.f(); g11<std::contracts::controls::enforced>(); }
