// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// In constant evaluation, a postcondition may take the address of a const
// by-value parameter or bind a reference to it, directly, through a call
// and in a lambda capture, including a constructor's postcondition; the
// parameter object is the one the body used.  The last static_assert is a
// violation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-postcondition-param-address.C
// in the gnu_gcc fork.  From the EDG fork's
// constexpr/postcondition_param_reference test (EDG-79).

constexpr bool positive(const int &y) { return y > 0; }
constexpr int a(const int x) post(r: *&x == r) { return x; }
constexpr int b(const int x) post(r: positive(x)) { return x; }  // expected-error {{contract failed during execution of constexpr function}}
constexpr int c(const int x) post(r: [&x, r] { return x == r; }()) { return x; }
constexpr int d(const int x, const int y) post(r: &x != &y) { return x + y; }
struct S {
  int m;
  constexpr S(const int x) post(positive(x)) : m(x) {}
};
static_assert(a(1) == 1);
static_assert(b(1) == 1);
static_assert(c(1) == 1);
static_assert(d(1, 2) == 3);
static_assert(S(1).m == 1);
static_assert(b(-1) == -1);  // expected-error {{static assertion expression is not an integral constant expression}}
