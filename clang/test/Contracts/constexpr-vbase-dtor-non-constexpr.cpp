// RUN: %clang_cc1 -std=c++26 -fsyntax-only -Winvalid-constexpr -verify %s

// The implicit destructor of a class with a virtual base whose destructor is
// not constexpr is not constexpr.  GCC ICEs explaining that (stock, gnu_gcc
// GCC-712, PR123681, deferred): its watch test reaches the ICE through
// -Winvalid-constexpr on `t`, and its bug-reports/ reproducer through the
// static_assert on `u`.  Clang does not crash: it does not warn on `t`
// (whether `t` can be constant depends on its argument) and rejects `u`,
// naming the non-constexpr destructor.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-vbase-dtor-explain-ice.C

struct B { virtual ~B(); }; // expected-note {{declared here}}
struct D : virtual B {};
constexpr int t(D *d) { d->D::~D(); return 0; }

constexpr int u() { D *d = new D(); delete d; return 0; } // expected-note {{non-constexpr function '~B' cannot be used in a constant expression}} \
                                                          // expected-note {{in call to}}
static_assert(u() == 0); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'u()'}}
