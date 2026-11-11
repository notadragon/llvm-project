// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=p3400
// REQUIRES: contracts-libcxx

// P4451 [basic.contract.control]: unqualified lookup of `terminating` from
// an assertion-control-specifier shall find nothing or only
// std::contracts::labels::terminating.  A parameter, a class member declared
// later (the specifier is a complete-class context), an enumerator, a
// namespace-scope variable, a block-scope variable, a lambda's parameter
// and an ambiguous lookup are rejected; a using-directive or
// using-declaration naming the standard one is not.  Under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone and both: under P3400
// the specifier is a label, the entity lookup finds, which is rejected for
// not being one (P3400 rejecting what P4451 rejects, with its own
// diagnostic).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-lookup.C (and its
// -p3400 and -both variants)

#include <contracts>  // `terminating`

namespace n1 {
void f(int terminating) // #n1
    pre<terminating>(true); // p4451-error {{unqualified lookup of 'terminating' from this assertion-control-specifier finds a declaration other than 'std::contracts::labels::terminating'}} p4451-note@#n1 {{declared here}} p3400-error {{not a valid assertion-control label}}
}

namespace n2 {
struct S {
  void f() pre<terminating>(true); // p4451-error {{finds a declaration other than}} p4451-note@#n2 {{declared here}} p3400-error {{not a valid assertion-control label}}
  static constexpr int terminating = 0; // #n2
};
}

namespace n3 {
enum E { terminating }; // #n3
void f() pre<terminating>(true); // p4451-error {{finds a declaration other than}} p4451-note@#n3 {{declared here}} p3400-error {{not a valid assertion-control label}}
}

namespace n4 {
constexpr int terminating = 0; // #n4
void f() pre<terminating>(true); // p4451-error {{finds a declaration other than}} p4451-note@#n4 {{declared here}} p3400-error {{not a valid assertion-control label}}
}

namespace n5 {
using namespace std::contracts::labels;
void f() pre<terminating>(true);
}

namespace n6 {
using std::contracts::labels::terminating;
void f() pre<terminating>(true);
}

namespace n7 {
namespace a { constexpr int terminating = 0; } // #n7a
namespace b { constexpr int terminating = 1; } // #n7b
using namespace a;
using namespace b;
void f() pre<terminating>(true); // p4451-error {{finds an ambiguous result instead of 'std::contracts::labels::terminating'}} p4451-note@#n7a {{declared here}} p4451-note@#n7b {{declared here}} p3400-error {{reference to 'terminating' is ambiguous}} p3400-note@#n7a {{candidate found by name lookup}} p3400-note@#n7b {{candidate found by name lookup}}
}

namespace n8 {
void g(int x) {
  int terminating = x; // #n8
  contract_assert<terminating>(x > 0); // p4451-error {{finds a declaration other than}} p4451-note@#n8 {{declared here}} p3400-error {{not a valid assertion-control label}}
}
auto l = [](int terminating) // #n8l
    pre<terminating>(true) { }; // p4451-error {{finds a declaration other than}} p4451-note@#n8l {{declared here}} p3400-error {{not a valid assertion-control label}}
}
