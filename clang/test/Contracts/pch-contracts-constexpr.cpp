// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -x c++-header -emit-pch -o %t/h.pch %t/h.h
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -include-pch %t/h.pch -fsyntax-only -verify %t/use.cpp

// Contract assertions declared in a precompiled header are checked in
// constant evaluation: a function's, a function template's instantiated only
// after the PCH (precondition and postcondition), a constructor's, a member
// function's precondition and contract_assert, and a lambda's.  A
// redeclaration after the PCH may repeat the contracts, also after a
// redeclaration in the header that omits them, and may not change them.
// (EDG shape: pch/constexpr.)
//
// Twelve more function templates follow, so that losing a template's
// contracts cannot pass by chance.
//
// GCC: gcc/testsuite/g++.dg/pch/contract-constexpr.C,
// g++.dg/pch/contract-template-after-pch.C and contract-redecl-after-pch.C
// (GCC-646, GCC-647).

//--- h.h
constexpr int cf(int x) pre(x > 0) { return x; }
template <class T> constexpr T ct(T x) pre(x > 0) post(r: r < 9) { return x; }
struct S {
  constexpr S(int x) pre(x > 0) : v(x) {}
  constexpr int mf(int x) const pre(x < v) { contract_assert(x != 2); return x; }
  int v;
};
constexpr auto lam = [](int x) pre(x > 0) { return x; };
int nf(int x) pre(x > 1);
int nf(int x);
int nf2(int x) pre(x > 1);
template <class T> constexpr T t0(T x) pre(x > 0) post(r: r < 9) { return x; }
template <class T> constexpr T t1(T x) pre(x > 1) post(r: r < 10) { return x; }
template <class T> constexpr T t2(T x) pre(x > 2) post(r: r < 11) { return x; }
template <class T> constexpr T t3(T x) pre(x > 3) post(r: r < 12) { return x; }
template <class T> constexpr T t4(T x) pre(x > 4) post(r: r < 13) { return x; }
template <class T> constexpr T t5(T x) pre(x > 5) post(r: r < 14) { return x; }
template <class T> constexpr T t6(T x) pre(x > 6) post(r: r < 15) { return x; }
template <class T> constexpr T t7(T x) pre(x > 7) post(r: r < 16) { return x; }
template <class T> constexpr T t8(T x) pre(x > 8) post(r: r < 17) { return x; }
template <class T> constexpr T t9(T x) pre(x > 9) post(r: r < 18) { return x; }
template <class T> constexpr T t10(T x) pre(x > 10) post(r: r < 19) { return x; }
template <class T> constexpr T t11(T x) pre(x > 11) post(r: r < 20) { return x; }

//--- use.cpp
// expected-error@h.h:1 {{contract failed during execution of constexpr function}}
// expected-error@h.h:2 2 {{contract failed during execution of constexpr function}}
// expected-error@h.h:4 {{contract failed during execution of constexpr function}}
// expected-error@h.h:5 2 {{contract failed during execution of constexpr function}}
// expected-error@h.h:8 {{contract failed during execution of constexpr function}}
static_assert(cf(1) == 1);
static_assert(cf(0) == 0);      // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(ct(1) == 1);
static_assert(ct(0) == 0);      // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(ct(9L) == 9);     // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(S(5).v == 5);
static_assert(S(0).v == 0);     // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(S(5).mf(4) == 4);
static_assert(S(5).mf(6) == 6); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(S(5).mf(2) == 2); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(lam(1) == 1);
static_assert(lam(0) == 0);     // expected-error {{not an integral constant expression}} expected-note {{in call to}}
int nf(int x) pre(x > 1);       // OK, the same contracts
int nf2(int x) pre(x > 2);      // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
// expected-note@h.h:11 {{contract previously specified with a non-equivalent condition}}
// Twelve more templates instantiated after the PCH (GCC-646's shape, where
// the PCH lost them depending on the build of the .gch).
static_assert(t0(1) == 1);
static_assert(t0(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t0(9L) == 9); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:12 2 {{contract failed during execution of constexpr function}}
static_assert(t1(2) == 2);
static_assert(t1(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t1(10L) == 10); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:13 2 {{contract failed during execution of constexpr function}}
static_assert(t2(3) == 3);
static_assert(t2(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t2(11L) == 11); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:14 2 {{contract failed during execution of constexpr function}}
static_assert(t3(4) == 4);
static_assert(t3(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t3(12L) == 12); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:15 2 {{contract failed during execution of constexpr function}}
static_assert(t4(5) == 5);
static_assert(t4(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t4(13L) == 13); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:16 2 {{contract failed during execution of constexpr function}}
static_assert(t5(6) == 6);
static_assert(t5(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t5(14L) == 14); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:17 2 {{contract failed during execution of constexpr function}}
static_assert(t6(7) == 7);
static_assert(t6(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t6(15L) == 15); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:18 2 {{contract failed during execution of constexpr function}}
static_assert(t7(8) == 8);
static_assert(t7(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t7(16L) == 16); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:19 2 {{contract failed during execution of constexpr function}}
static_assert(t8(9) == 9);
static_assert(t8(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t8(17L) == 17); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:20 2 {{contract failed during execution of constexpr function}}
static_assert(t9(10) == 10);
static_assert(t9(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t9(18L) == 18); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:21 2 {{contract failed during execution of constexpr function}}
static_assert(t10(11) == 11);
static_assert(t10(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t10(19L) == 19); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:22 2 {{contract failed during execution of constexpr function}}
static_assert(t11(12) == 12);
static_assert(t11(0) == 0);   // expected-error {{not an integral constant expression}} expected-note {{in call to}}
static_assert(t11(20L) == 20); // expected-error {{not an integral constant expression}} expected-note {{in call to}}
// expected-error@h.h:23 2 {{contract failed during execution of constexpr function}}
