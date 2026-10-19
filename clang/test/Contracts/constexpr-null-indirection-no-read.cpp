// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Indirection through a null pointer is undefined ([expr.unary.op]/1,
// CWG2823) whether or not the result is read, so it is not a core constant
// expression.  Clang rejects a discarded `*p`, `&*p` and `(*sp).sf()`, a
// static member function called through an explicit indirection (Clang 22
// on; Clang 18 to 21 accepted all three).  GCC accepts the discarded
// `*p` (stock, gnu_gcc GCC-749, deferred), and its watch test is f1 and the
// arrow form `sp->sf()`.  The arrow forms, `sp->sf()` and `sp->k` for a
// static data member, Clang accepts too: that is CLANG-737
// (OpenBugs/constexpr-null-static-member-call.cpp).  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-null-indirection-no-read.C

struct S { static constexpr int sf() { return 1; } };
constexpr bool f1() {
  int *p = nullptr;
  *p; // expected-warning {{expression result unused}} \
      // expected-note {{dereferencing a null pointer is not allowed in a constant expression}}
  return true;
}
static_assert(f1()); // expected-error {{not an integral constant expression}} \
                     // expected-note {{in call to 'f1()'}}

constexpr bool f3() {
  int *p = nullptr;
  int *q = &*p; // expected-note {{dereferencing a null pointer is not allowed in a constant expression}}
  return q == nullptr;
}
static_assert(f3()); // expected-error {{not an integral constant expression}} \
                     // expected-note {{in call to 'f3()'}}

constexpr bool f4() {
  S *sp = nullptr;
  (void)(*sp).sf(); // expected-note {{dereferencing a null pointer is not allowed in a constant expression}}
  return true;
}
static_assert(f4()); // expected-error {{not an integral constant expression}} \
                     // expected-note {{in call to 'f4()'}}
