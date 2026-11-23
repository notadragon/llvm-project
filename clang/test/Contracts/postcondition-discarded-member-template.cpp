// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -Wno-unused-value -fsyntax-only -verify %s

// A member access naming a static data member, a static member function or
// an enumerator odr-uses its object, which is not among its potential results
// ([basic.def.odr]/3, /5), so a by-value parameter named only as that object
// in a postcondition must be const ([dcl.contract.func]); not inside
// `sizeof', and not as a lambda's own parameter.  The same through a function
// template's dependent object, and across a template's declarations.  The
// non-static data member, which does not odr-use its object, is
// postcondition-discarded-member-access.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-discarded-member-template.C
// in the gnu_gcc fork (GCC-663, GCC-666), whose CLANG-647 row asked for it.
// GCC diagnoses st_re at its definition, the declaration that is const; Clang
// at the declaration with the postcondition, which is not.

struct S { int m; };
struct T { static int sm; static bool sf(); enum { E = 1 }; int m; };

template <class U> void re_tmpl(const U s) post((s.m, true));
template <class U> void re_tmpl(U s) {}               // OK
template void re_tmpl<S>(S);

template <class U> void mem(U s) post((s.m, true)) {} // OK
template void mem<S>(S);

// The static member through the dependent object.
template <class U> void st(U s) post((s.sm, true)) {} // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
template void st<T>(T);                               // expected-note {{in instantiation of}}
template <class U> void st_re(U s) post((s.sm, true)); // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
template <class U> void st_re(const U s) {}
template void st_re<T>(T);                            // expected-note {{in instantiation of}}

// And without a template.
void ns_sm(T s) post((s.sm, true)) {}   // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
void ns_sm2(T s) post(s.sm > 0) {}      // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
void ns_sf(T s) post(s.sf()) {}         // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
void ns_e(T s) post(s.E > 0) {}         // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}
void ns_sizeof(T s) post(sizeof(s.sm) > 0) {}                   // OK, unevaluated
void ns_const(const T s) post(s.sm > 0 && s.sf() && s.E > 0) {} // OK
void ns_lambda(T t) post([](T s) { return s.sm > 0; }(T{})) {}  // OK, the lambda's own
