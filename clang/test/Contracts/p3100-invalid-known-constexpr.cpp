// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:enforce

// P3100: with the past-the-end indirection check configured, constant
// evaluation is unchanged: an indirection through known-bound pointer
// arithmetic within the array is still a constant expression, and a
// reference bound to `*(a + N)` and a read through nested arithmetic
// `*(a + 1 + 2)` are rejected by the evaluator as they always were.  The
// address of the indirection `&*(a + N)` is accepted, configured or not
// (CLANG-712, deferred; watch test
// OpenBugs/constexpr-address-past-end-indirection.cpp).  (GCC's evaluator
// accepts the address and the reference forms, so there the check reports
// all three as undefined.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-known-constexpr.C)

constexpr bool f(int i) { int a[3]{}; int *p = &*(a + i); return p != nullptr; }
static_assert(f(2));
constexpr int g(int i) { int a[3]{1, 2, 3}; return *(a + 1 + i); } // expected-note {{read of dereferenced one-past-the-end pointer is not allowed in a constant expression}}
static_assert(g(1) == 3);
constexpr int b2 = g(2); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'g(2)'}}
constexpr int h(int i) { int a[3]{1, 2, 3}; int &r = *(a + i); return &r - a; } // expected-note {{read of dereferenced one-past-the-end pointer is not allowed in a constant expression}}
static_assert(h(2) == 2);
constexpr int b3 = h(3); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'h(3)'}}
