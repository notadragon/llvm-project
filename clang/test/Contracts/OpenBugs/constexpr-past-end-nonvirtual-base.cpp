// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// XFAIL: *
// expected-no-diagnostics

// OPEN BUG (CLANG-689 in this fork's bug-reports/, llvm#43201; deferred by
// the user 2026-10-12, DECISIONS.md E73).  Clang only: GCC accepts it.
//
// Converting a past-the-end pointer to a non-virtual base class is a
// constant expression: the conversion needs no object, only an offset
// ([conv.ptr]/3).  Clang rejects it ("cannot access base class of pointer
// past the end of object").  When it is accepted, this XFAIL becomes an
// XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/constexpr-past-end-nonvirtual-base.C)

struct B { int b = 1; };
struct NV : B { int x = 2; };
constexpr bool t() {
  NV ds[1];
  NV *p = ds + 1;
  B *b = p;
  return b != nullptr;
}
static_assert(t());
