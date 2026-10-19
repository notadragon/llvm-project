// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s \
// RUN:   -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:noexcept_observe
// XFAIL: *

// OPEN BUG (CLANG-779 in this fork's bug-reports/).  Stock Clang 18 to
// trunk.
//
// `a[2]` on a two-element array is an indirection through a pointer that
// points to no object, which is undefined ([expr.unary.op]/1), so a copy
// from it is not a core constant expression even when the class is empty
// and the trivial copy reads nothing.  Clang accepts an empty class
// copied, moved or assigned from the element one past the end: the
// evaluator skips the read of an empty class
// (isReadByLvalueToRvalueConversion), and with it the check of the
// designated object.  A store to the element and a copy of a non-empty
// class from it are rejected (the controls, which pass today).  When fixed,
// this XFAIL becomes an XPASS, the signal to close the bug.  The second RUN
// line configures the P3100 bounds checks, which leave constant evaluation
// unchanged (DECISIONS.md E95 (a) makes these copies run-time
// .invalid.known checks).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-array-bounds-copy-constexpr.C,
// the copy and the move, which GCC rejects.)

struct E {};
struct S { int x; };

constexpr bool copy(int i) { E a[2]; E e2 = a[i]; (void)e2; return true; }
static_assert(copy(1));
static_assert(copy(2)); // expected-error {{not an integral constant expression}}

constexpr bool move(int i) { E a[2]; E e2 = static_cast<E &&>(a[i]); (void)e2; return true; }
static_assert(move(1));
static_assert(move(2)); // expected-error {{not an integral constant expression}}

constexpr bool from(int i) { E a[2]; E e; e = a[i]; return true; }
static_assert(from(1));
static_assert(from(2)); // expected-error {{not an integral constant expression}}

// Controls, which pass: the store to it and a non-empty copy are rejected.
constexpr bool store(int i) { E a[2]; E e; a[i] = e; return true; }
static_assert(store(2)); // expected-error {{not an integral constant expression}}

constexpr bool scopy(int i) { S a[2]{}; S s2 = a[i]; (void)s2; return true; }
static_assert(scopy(2)); // expected-error {{not an integral constant expression}}
