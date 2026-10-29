// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -Wno-unused-value %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -Wno-unused-value %s \
// RUN:   -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:observe
// XFAIL: *

// OPEN BUG (CLANG-756 in this fork's bug-reports/).  Stock Clang 18 to
// trunk.
//
// `*fp` with `fp` a null pointer to function is an indirection through a
// null pointer, undefined by [expr.unary.op]/1 whatever is done with the
// result, so none of these calls is a core constant expression; Clang
// accepts a reference bound to it, its address and a discarded `*fp`
// (LValueExprEvaluator::VisitUnaryDeref checks the pointer only for an
// object type).  The object forms and a call through the null pointer are
// rejected (the controls, which pass today).  When the function forms are
// rejected too, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-null-indirection-no-read.C;
// GCC rejects the reference binding and accepts the other two, and accepts
// the object `&*p`: GCC-749, deferred.)  The second RUN line configures the
// P3100 check of these indirections (CLANG-753), which leaves manifestly
// constant evaluation unchanged.

using F = int();
struct S { int m; };

constexpr int bind(F *fp) { F &r = *fp; (void)r; return 1; }
static_assert(bind(nullptr) == 1); // expected-error {{not an integral constant expression}}

constexpr bool addr(F *fp) { return &*fp == nullptr; }
static_assert(addr(nullptr)); // expected-error {{not an integral constant expression}}

constexpr int disc(F *fp) { *fp; return 1; }
static_assert(disc(nullptr) == 1); // expected-error {{not an integral constant expression}}

// Controls.
constexpr int call(F *fp) { return (*fp)(); }
static_assert(call(nullptr) == 1); // expected-error {{not an integral constant expression}}

constexpr int obind(S *p) { S &r = *p; (void)r; return 1; }
static_assert(obind(nullptr) == 1); // expected-error {{not an integral constant expression}}

constexpr bool oaddr(S *p) { return &*p == nullptr; }
static_assert(oaddr(nullptr)); // expected-error {{not an integral constant expression}}
