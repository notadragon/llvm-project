// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0 ("Taking the Address of a Function"; DECISIONS.md E146 (a)): the
// definition of an object of static or thread storage duration is treated
// as taking the address of its destructor, which the implementation calls
// at exit or when the thread ends, with no caller in the program to analyze
// the call.  So a precondition of the destructor that allows no runtime
// semantic, and that folding does not prove, cannot be proven for that
// call: an error under prove (always_prove) and a -Wcontract-unproven
// warning under diagnose (always_diagnose), at namespace scope, a static
// local, a static data member's definition (out of line, or inline),
// thread_local objects, and arrays of such a class.  Not diagnosed: a
// declaration that is not a definition, an automatic object (its
// destructor call is analyzed where it is made), a destructor whose
// precondition is not analyzed, or is tautological (`DC', `pre (true)'), an
// object never destroyed ([[clang::no_destroy]]) or with constant
// destruction, and a template's definition -- its instantiations are, each
// once.  (CLANG-861: no such definition was diagnosed.)
//
// Constant destruction (DECISIONS.md E149 (a), the Divergences row
// CLANG-865): Clang does not destroy a non-constexpr object whose
// destruction is a constant expression, so `DE ce2;' -- whose destructor
// precondition holds in constant evaluation (is_constant_evaluated()) but
// is not proven by folding outside it -- is not diagnosed; gnu_gcc
// evaluates constant destruction only for a constexpr variable, so it
// registers ce2's destructor and diagnoses it.  Clang's constant
// destruction of a non-constexpr object cannot read its members, so
// `DM m2;' (a precondition reading a member, which folding cannot prove)
// is destroyed through the address and diagnosed by both.  A destructor
// precondition that is not a core constant expression makes the
// destruction non-constant (CLANG-862), so `DG g1;' is destroyed through
// the address and diagnosed.
// Temporaries of static storage duration are
// p4405-analysis-address-static-temp.cpp; the code generation of arrays,
// p4405-analysis-static-array-dtor.cpp.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int g;

struct DP {
  ~DP() pre<always_prove>(g > 0); // expected-note 10 {{precondition here}}
};
struct DD {
  ~DD() pre<always_diagnose>(g > 0); // expected-note 2 {{precondition here}}
};
struct DI {
  ~DI() pre(g > 0);
};
struct DC {
  constexpr ~DC() pre<always_prove>(true) {}
};
struct DM {
  int v = 1;
  constexpr ~DM() pre<always_prove>(v > 0) {} // expected-note {{precondition here}}
};
struct DE {
  constexpr ~DE() pre<always_prove>(__builtin_is_constant_evaluated() || g > 0) {}
};
struct DG {
  constexpr ~DG() pre<always_prove>(g > 0) {} // expected-note {{precondition here}}
};

DP p1;  // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'p1', of static storage duration, is destroyed by a call through the address of its destructor}}
DD d1;  // expected-warning {{contract assertion cannot be proven for a call through the address of '~DD'}} expected-note {{'d1', of static storage duration, is destroyed by a call through the address of its destructor}}
DI i1;
DP pa[2]; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'pa', of static storage duration, is destroyed by a call through the address of its destructor}}
thread_local DP t1; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'t1', of thread storage duration, is destroyed by a call through the address of its destructor}}
thread_local DD t2; // expected-warning {{contract assertion cannot be proven for a call through the address of '~DD'}} expected-note {{'t2', of thread storage duration, is destroyed by a call through the address of its destructor}}
extern DP e1;
[[clang::no_destroy]] DP nd;
constexpr DC c1;
DC c2;
constexpr DM m1;
DM m2; // expected-error {{contract assertion cannot be proven for a call through the address of '~DM'}} expected-note {{'m2', of static storage duration, is destroyed by a call through the address of its destructor}}
constexpr DE ce1;
DE ce2;
DG g1; // expected-error {{contract assertion cannot be proven for a call through the address of '~DG'}} expected-note {{'g1', of static storage duration, is destroyed by a call through the address of its destructor}}

struct S {
  static DP s1;
  static inline DP s2; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'s2', of static storage duration, is destroyed by a call through the address of its destructor}}
};
DP S::s1; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'s1', of static storage duration, is destroyed by a call through the address of its destructor}}

void f() {
  static DP sl; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'sl', of static storage duration, is destroyed by a call through the address of its destructor}}
  thread_local DP tl; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'tl', of thread storage duration, is destroyed by a call through the address of its destructor}}
  DP a;
  DI ai;
}

template <class T> void tf() {
  static DP st; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'st', of static storage duration, is destroyed by a call through the address of its destructor}}
}
template <class T> void never() { static DP sn; }

template <class T> struct TS {
  static inline DP si; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'si', of static storage duration, is destroyed by a call through the address of its destructor}}
};

template <class T> DP vt; // expected-error {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'vt<int>', of static storage duration, is destroyed by a call through the address of its destructor}}

void use() {
  tf<int>(); // expected-note-re {{in instantiation of {{.*}} requested here}}
  tf<int>();
  (void)&TS<int>::si; // expected-note-re {{in instantiation of {{.*}} requested here}}
  (void)&vt<int>; // expected-note-re {{in instantiation of {{.*}} requested here}}
}
