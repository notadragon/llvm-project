// RUN: %clang_cc1 -std=c++20 -fcxx-exceptions -fsyntax-only -verify %s
// expected-no-diagnostics

// An array element an initializer list does not initialize explicitly is
// initialized by the list's array filler, which is not among the list's
// children; a potentially-throwing filler makes the list potentially-throwing
// ([except.spec]/6.2).

struct S { S() { throw 2; } };
struct N { N() noexcept; };
struct A { S s[2]; };
struct B { N n[2]; };
struct C { int i; S s[2]; };

static_assert(!noexcept(A{}));
static_assert(!noexcept(A{{}}));
static_assert(!noexcept(C{1}));
using S2 = S[2];
static_assert(!noexcept(S2{}));

static_assert(noexcept(B{}));
using N2 = N[2];
static_assert(noexcept(N2{}));
using I4 = int[4];
static_assert(noexcept(I4{1}));

template <class T> constexpr bool fill() { return noexcept(T{}); }
static_assert(!fill<A>());
static_assert(fill<B>());
