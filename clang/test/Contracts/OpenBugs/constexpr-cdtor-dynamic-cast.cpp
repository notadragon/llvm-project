// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-714 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-747, PR93096).
//
// A dynamic_cast during construction whose operand is not the object under
// construction or one of its bases, here reaching Bx through A, is undefined
// ([class.cdtor]/6, the standard's own example), so the static_assert must
// be an error; Clang accepts it.  The conversion form of the row is
// constexpr-cdtor-convert-virtual-base.cpp.  When it is rejected, this XFAIL
// becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-cdtor-dynamic-cast.C)

struct V { virtual void f() {} };
struct A : virtual V {};
struct Bx : virtual V {
  bool ok;
  constexpr Bx(A *a) : ok(dynamic_cast<Bx *>(a) != nullptr) {}
};
struct D : A, Bx { constexpr D() : Bx(this) {} };
static_assert((D{}, true)); // expected-error {{not an integral constant expression}}
