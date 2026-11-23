// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// XFAIL: *
// expected-no-diagnostics

// OPEN BUG (CLANG-725 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Clang only: GCC evaluates it correctly.
//
// During construction of Bx, a dynamic_cast to Bx of a pointer to its
// virtual base V refers to the object under construction ([class.cdtor]/6)
// and yields `this`.  Clang's constant evaluation gives a different result,
// so the static_assert fails.  When it holds, this XFAIL becomes an XPASS,
// the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/constexpr-dynamic-cast-vbase-construction.C)

struct V { virtual void f() {} };
struct Bx : virtual V {
  bool good;
  constexpr Bx() { V *v = this; good = (dynamic_cast<Bx *>(v) == this); }
};
struct D : Bx { constexpr D() {} };
constexpr bool t1() { D d; return d.good; }
static_assert(t1());
