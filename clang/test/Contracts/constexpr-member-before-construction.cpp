// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Controls for OpenBugs/address-before-ctor-member.cpp (CLANG-9): reading a
// member whose non-trivial constructor has not begun is diagnosed in
// constant evaluation; forming the object's own address, a member with a
// trivial constructor, an already-built member and use from the constructor
// body are fine ([class.cdtor]).  No contracts involved (the EDG fork's
// EDG-45 shapes).
//
// Mirror: the `controls` namespace of
// gcc/testsuite/g++.dg/contracts/cpp26/open-bug-address-before-ctor.C.

struct Z {
  int k;
  constexpr Z() : k(0) {}
};
struct Y2 {
  int q;
  Z z;
  constexpr Y2() : q(z.k) {} // expected-warning {{field 'z' is uninitialized when used here}} \
                             // expected-note {{read of object outside its lifetime is not allowed in a constant expression}}
};
constexpr int f2() { Y2 y; return y.q; } // expected-note {{in call to 'Y2()'}} expected-note {{declared here}}
static_assert((f2(), true)); // expected-error {{static assertion expression is not an integral constant expression}} \
                             // expected-note {{in call to 'f2()'}}
struct Y4 { Z *p; Z z; constexpr Y4() : p(&z) {} };
static_assert((Y4(), true));
struct T { int k; };
struct Y5 { int *p; T t; constexpr Y5() : p(&t.k), t{1} {} };
static_assert((Y5(), true));
struct Y6 { Z z; int *p; constexpr Y6() : p(&z.k) {} };
static_assert((Y6(), true));
struct Y7 { Z z; int k; constexpr Y7() : k(0) { z.k = 1; } };
static_assert((Y7(), true));
