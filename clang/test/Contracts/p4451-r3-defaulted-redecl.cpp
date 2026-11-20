// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -Xclang -verify=expected,p4451
// RUN: %{cc} -fcontracts-p3400 -Xclang -verify=expected,p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify=expected,p3400
// REQUIRES: contracts-libcxx

// P4451R3 [dcl.contract.func]: two function contract assertions are the
// same only if both have an assertion-control-specifier or neither does,
// and, if both do, their id-expressions denote the same entity.  So an
// unlabeled assertion and one naming defaulted differ although they mean
// the same (f, g), defaulted and enforced differ (h, k), and two spellings
// of defaulted are the same (s, u::t, S::n).  Under -fcontracts-p4451 alone,
// -fcontracts-p3400 alone (where the specifier is the P3400 label, so the
// labels differ, and two spellings of it are the same expression) and both.

#include <contracts>  // `std::contracts::controls::defaulted`

namespace scl = std::contracts::controls;

void f(int x) pre(x > 0); // p4451-note {{contract previously specified without an assertion-control-specifier}} p3400-note {{contract previously specified with a different assertion-control label}}
void f(int x) pre<scl::defaulted>(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

void g(int x) pre<std::contracts::controls::defaulted>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}} p3400-note {{contract previously specified with a different assertion-control label}}
void g(int x) pre(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

void h(int x) pre<scl::defaulted>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}} p3400-note {{contract previously specified with a different assertion-control label}}
void h(int x) pre<scl::enforced>(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

void k(int x) pre<scl::enforced>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::enforced'}} p3400-note {{contract previously specified with a different assertion-control label}}
void k(int x) pre<scl::defaulted>(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

// The second assertion differs; the first matches.
void p(const int x) pre<scl::defaulted>(x > 0) post<scl::defaulted>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}} p3400-note {{contract previously specified with a different assertion-control label}}
void p(const int x) pre<scl::defaulted>(x > 0) post(x > 0) { } // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}

void s(const int x) pre<std::contracts::controls::defaulted>(x > 0) post<scl::defaulted>(x > 0);
void s(const int x) pre<scl::defaulted>(x > 0) post<::std::contracts::controls::defaulted>(x > 0) { }

namespace u {
using namespace std::contracts::controls;
void t(int x) pre<defaulted>(x > 0);
}
void u::t(int x) pre<std::contracts::controls::defaulted>(x > 0) { }

struct S {
  void m(int x) pre<scl::defaulted>(x > 0); // p4451-note {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}} p3400-note {{contract previously specified with a different assertion-control label}}
  void n(int x) pre<scl::defaulted>(x > 0);
};
void S::m(int x) pre(x > 0) { } // expected-error {{method out-of-line definition differs in contract specifier sequence}} expected-note {{in contract specified here}}
void S::n(int x) pre<std::contracts::controls::defaulted>(x > 0) { }
