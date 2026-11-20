// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -Wno-unused-value -fsyntax-only -verify %s

// A by-value parameter named as the object expression of a discarded member
// access in a postcondition is not odr-used, so it need not be const
// ([basic.def.odr]: the potential results of a member access naming a
// non-static data member include those of its object expression;
// [dcl.contract.func]) (CLANG-637).  The discarded parameter itself is
// Sema/pr126897.cpp.  A volatile member access, to which the lvalue-to-rvalue
// conversion is applied, still odr-uses it, as does a call.
//
// A redeclaration without const, and the declarations of a function
// template, are not reached by such a parameter either (CLANG-646: the
// redeclaration check counted every naming of the parameter as an odr-use).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-discarded-member-access.C
// in the gnu_gcc fork (GCC-650).  Found by the EDG fork's
// sema/postcondition_discarded_params test.

struct S { int m; bool ok() const; };
struct T { S in; };

void mem(S s) post((s.m, true)) {}                    // OK
void mem_void(S s) post(((void)s.m, true)) {}         // OK
void mem_nested(T t) post((t.in.m, true)) {}          // OK
void whole(S s) post((s, true)) {}                    // OK, as Sema/pr126897.cpp
void call(S s) post((s.ok(), true)) {}  // expected-error {{parameter 's' referenced in contract postcondition must be declared const}} expected-note {{declared here}}

struct V { volatile int m; };
void vol(V v) post((v.m, true)) {}      // expected-error {{parameter 'v' referenced in contract postcondition must be declared const}} expected-note {{declared here}}

// Redeclarations (CLANG-646).
void re_whole(S s) post((s, true));
void re_whole(S s);                                   // OK
void re_mem(S s) post((s.m, true));
void re_mem(S s);                                     // OK
template <class U> void re_tmpl(const U s) post((s.m, true));
template <class U> void re_tmpl(U s) {}               // OK
template void re_tmpl<S>(S);
void re_call(const S s) post((s.ok(), true)); // expected-note {{odr-used in a postcondition here}}
void re_call(S s);                      // expected-error {{parameter 's' referenced in contract postcondition must be declared const}}
