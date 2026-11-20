// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// REQUIRES: contracts-libcxx

// P4451 [basic.contract.control] (P4451R2): name lookup for the
// id-expression of an assertion-control-specifier shall find the standard
// library declaration of std::contracts::controls::enforced.  An
// unqualified `enforced` that finds a parameter, a class member declared
// later (the specifier is a complete-class context), an enumerator, a
// namespace-scope variable, a block-scope variable, a lambda's parameter, or
// an ambiguous result is rejected; a using-directive or using-declaration
// naming the standard one is not.  Under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone and both: under P3400 the specifier is a label,
// the entity lookup finds, which is rejected for not being one (P3400
// rejecting what P4451 rejects, with its own diagnostic).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-lookup.C (and its
// -p3400 and -both variants)

#include <contracts>  // `std::contracts::controls::enforced`

namespace n1 {
void f(int enforced) // #n1
    pre<enforced>(true); // p4451-error {{assertion-control-specifier must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup finds another declaration}} p4451-note@#n1 {{declared here}} p3400-error {{not a valid assertion-control label}}
}

namespace n2 {
struct S {
  void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#n2 {{declared here}} p3400-error {{not a valid assertion-control label}}
  static constexpr int enforced = 0; // #n2
};
}

namespace n3 {
enum E { enforced }; // #n3
void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#n3 {{declared here}} p3400-error {{not a valid assertion-control label}}
}

namespace n4 {
constexpr int enforced = 0; // #n4
void f() pre<enforced>(true); // p4451-error {{name lookup finds another declaration}} p4451-note@#n4 {{declared here}} p3400-error {{not a valid assertion-control label}}
}

namespace n5 {
using namespace std::contracts::controls;
void f() pre<enforced>(true);
}

namespace n6 {
using std::contracts::controls::enforced;
void f() pre<enforced>(true);
}

namespace n7 {
namespace a { constexpr int enforced = 0; } // #n7a
namespace b { constexpr int enforced = 1; } // #n7b
using namespace a;
using namespace b;
void f() pre<enforced>(true); // p4451-error {{must name 'std::contracts::controls::enforced' or 'std::contracts::controls::defaulted'; name lookup is ambiguous}} p4451-note@#n7a {{declared here}} p4451-note@#n7b {{declared here}} p3400-error {{reference to 'enforced' is ambiguous}} p3400-note@#n7a {{candidate found by name lookup}} p3400-note@#n7b {{candidate found by name lookup}}
}

namespace n8 {
void g(int x) {
  int enforced = x; // #n8
  contract_assert<enforced>(x > 0); // p4451-error {{name lookup finds another declaration}} p4451-note@#n8 {{declared here}} p3400-error {{not a valid assertion-control label}}
}
auto l = [](int enforced) // #n8l
    pre<enforced>(true) { }; // p4451-error {{name lookup finds another declaration}} p4451-note@#n8l {{declared here}} p3400-error {{not a valid assertion-control label}}
}
