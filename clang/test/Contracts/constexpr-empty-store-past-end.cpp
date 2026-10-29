// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note -Wno-array-bounds %s

// `a[1]` on a one-element array is an indirection through a pointer that
// points to no object, which is undefined ([expr.unary.op]/1), so an empty
// class stored to it, `a[1] = e`, is not a core constant expression even
// though the trivial assignment reads and writes nothing.  Clang rejects it;
// GCC accepts it (stock, gnu_gcc GCC-831, deferred), and its watch test is
// the program below.  Not contracts-specific.  GCC's control, the empty copy
// from it, which GCC rejects, is accepted by Clang: CLANG-779, whose watch
// test is OpenBugs/constexpr-empty-copy-past-end.cpp.  The control here is
// a non-empty class copied from it.  So is an empty base of a non-empty
// element stored to, or assigned from (`static_cast<EB &>(da[1]) = b`,
// `b = da[1]`), which GCC accepts too (GCC-831); an empty class assigned
// from it, `e = a[1]`, GCC's other form, is CLANG-779's (its watch test's
// `from`).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-empty-store-past-end.C

struct E {};
constexpr bool f() {
  E a[1];
  E e;
  a[1] = e;
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}}

struct EB {};
struct D : EB { int x; };
constexpr bool bstore() {
  D da[1]{};
  EB b;
  static_cast<EB &>(da[1]) = b;
  return true;
}
static_assert(bstore()); // expected-error {{not an integral constant expression}}

constexpr bool bfrom() {
  D da[1]{};
  EB b;
  b = da[1];
  return true;
}
static_assert(bfrom()); // expected-error {{not an integral constant expression}}

struct S { int x; };
constexpr bool g() { S a[1]{}; S s2 = a[1]; (void)s2; return true; }
static_assert(g()); // expected-error {{not an integral constant expression}}
