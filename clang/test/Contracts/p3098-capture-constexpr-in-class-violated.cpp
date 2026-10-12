// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// P3098 x constant evaluation: a violated postcondition capture in a member
// function defined in its class is diagnosed during constant evaluation --
// the capture is bound in the callee's frame, so the predicate reads the
// entry value, not the member as modified by the body.  The holding cases
// for in-class members are in p3098-capture-constexpr-templates.cpp; this
// file pins the violated ones (CLANG-100).

struct S {
  int v = 0;
  constexpr int same(int x) post [x] (r: r == x) { return x + 1; } // expected-error {{contract failed during execution of constexpr function}}
  constexpr void add(const int x) post [old = v] (v == old) { v += x; } // expected-error {{contract failed during execution of constexpr function}}
};

constexpr int bad1 = S{}.same(5); // expected-error {{constexpr variable 'bad1' must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int bad2 = [] { S s; s.add(3); return s.v; }(); // expected-error {{constexpr variable 'bad2' must be initialized by a constant expression}} expected-note 2 {{in call to}}

// The capture holds the entry value: a body that changes the member back
// keeps the postcondition true.
struct T {
  int v = 1;
  constexpr int roundtrip() post [old = v] (r: r == old * 2 && v == old) {
    int was = v;
    v = 100;
    v = was;
    return was * 2;
  }
};
static_assert(T{}.roundtrip() == 2);
