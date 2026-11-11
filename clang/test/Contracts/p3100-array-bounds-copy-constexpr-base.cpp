// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -Wno-array-bounds %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -Wno-array-bounds %s \
// RUN:   -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:noexcept_observe

// P3100: with the bounds checks configured, an empty base of a non-empty
// element copied from the element one past the end of an array of known
// bound, `EB b = da[2]`, is diagnosed in constant evaluation as it is
// without them: the run-time check that keeps the element's address for
// the elided copy (DECISIONS.md E95 (a), E98 (a)) leaves constant
// evaluation unchanged.  An element in range is a constant expression.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-array-bounds-copy-constexpr.C,
// its base copy; its empty-class copy and move, which Clang accepts
// (CLANG-779), are that bug's watch test,
// OpenBugs/constexpr-empty-copy-past-end.cpp.

struct EB {};
struct D : EB { int x; };
constexpr bool bcopy(int i) { D a[2]{}; EB b = a[i]; (void)b; return true; }
static_assert(bcopy(1));
static_assert(bcopy(2)); // expected-error {{not an integral constant expression}}
