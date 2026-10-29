// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): when an aggregate's initialization
// throws part-way, the members already built are destroyed; that cleanup is
// a destructor call made in the initialization, so it has an analysis
// evaluation whatever the aggregate's storage duration: in a function, for
// a static local, at namespace scope and for a static data member (the
// static initialization function makes the call).  The member's own
// initializer is elided (`DP()', `make()'), so it adds nothing.  A's
// implicit destructor's call of `~DP' is analyzed in A's destructor, once.
// Without optimization (folding) and with it (the proof shadow), where
// nothing can prove `g > 0'.  Code generation, not -fsyntax-only.
//
// gnu_gcc's test uses always_prove; here the precondition is
// always_diagnose, as in p4405-analysis-elided-temporary.cpp, so every
// diagnostic is a -Wcontract-unproven warning.  `~A' has no precondition of
// its own, so the variables of static storage duration get no address-of
// diagnostic; Clang gives A's implicit destructor no "synthesized method"
// note.
//
// The member's destructor is potentially invoked ([dcl.init.aggr]) even when
// nothing after it can throw (`A e1{DP(), 1};', a `noexcept' call after it):
// Clang then emits no partial-destruction cleanup, but the call is analyzed
// all the same, as gnu_gcc does (notadragon_wg21 DECISIONS.md E160 (a)) --
// for a base, a member initialized by parentheses, a lambda's capture and an
// array's element too.  A later initialization is one the initializer or a
// default member initializer gives; a member left to implicit
// value-initialization (`A e6{DP()};') is not one, and the last member
// (`L e7{1, DP()};') has nothing after it.  A base or an array element built
// by no expression of its own is diagnosed at the end of the function or at
// the variable.  So is an array member (`AR b10{{DN(), DN()}, 1};'), its
// elements destroyed one by one: in `h' that evaluation would be the one
// already diagnosed for `b9', so `b10' has a function of its own.  Analysis
// only: p4405-analysis-elided-temporary-aggregate-same-code.cpp shows the
// code is unchanged.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-elided-temporary-aggregate.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int g;

struct DP {
  DP();
  ~DP() pre<always_diagnose>(g > 0); // expected-note 22 {{the predicate of the contract assertion}}
};
DP make();
int thr();
struct A { DP p; int i; }; // expected-warning {{contract assertion cannot be proven}}

// Namespace scope.
A na{DP(), thr()}; // expected-warning {{contract assertion cannot be proven}}
A nb{make(), thr()}; // expected-warning {{contract assertion cannot be proven}}
struct SA { static A sa; };
A SA::sa{DP(), thr()}; // expected-warning {{contract assertion cannot be proven}}

// In a function.
void f() {
  A b{DP(), thr()}; // expected-warning {{contract assertion cannot be proven}}
  static A c{DP(), thr()}; // expected-warning {{contract assertion cannot be proven}}
  (void)b;
  (void)c;
}

// E160 (a): nothing after the member can throw.
int nothr() noexcept;
struct DN {
  DN() noexcept;
  ~DN() pre<always_diagnose>(g > 0); // expected-note 6 {{the predicate of the contract assertion}} expected-note 2 {{precondition here}}
};
struct AI { DP p; int i = 5; }; // expected-warning {{contract assertion cannot be proven}}
struct L { int i; DP p; }; // expected-warning {{contract assertion cannot be proven}}
// D declares its destructor (an aggregate may), defined in no TU here, so
// only its base's partial cleanup is analyzed; an implicit one would be an
// alias of `~DP' with optimization, and never analyzed.
struct D : DP { int i; ~D(); };
// An array member's elements are destroyed in a loop.
struct AR { DN a[2]; int i; }; // expected-warning {{contract assertion cannot be proven}}
// A nested aggregate with nothing after its member `a' (`e10'): `a.p''s
// cleanup is not potentially invoked.  With `j' after it (`e9'), Clang's
// partial cleanup destroys `a' with `~A', which has no precondition; the
// call of `~DP' in A's implicit destructor is analyzed there, once, so
// nothing is diagnosed at `e9'.  An accepted divergence: gnu_gcc's cleanup
// destroys `a.p' with `~DP' directly and diagnoses it at `e9'
// (notadragon_wg21 DECISIONS.md E161 (a); open-issues/ CLANG-876).
struct O { A a; int j; };

A e1{DP(), 1}; // expected-warning {{contract assertion cannot be proven}}
A e2{DP(), nothr()}; // expected-warning {{contract assertion cannot be proven}}
A e3(DP(), 1); // expected-warning {{contract assertion cannot be proven}}
D e4{{}, 1}; // expected-warning {{contract assertion cannot be proven}}
AI e5{DP()}; // expected-warning {{contract assertion cannot be proven}}
A e6{DP()};
L e7{1, DP()};
struct SE { static A se; };
A SE::se{DP(), 1}; // expected-warning {{contract assertion cannot be proven}}
DN e8[2] = {DN(), DN()}; // expected-warning 2 {{contract assertion cannot be proven}} expected-note {{'e8', of static storage duration, is destroyed by a call through the address of its destructor}}
O e9{{DP()}, 1};
O e10{{DP()}};

void h() {
  A b1{DP(), 1}; // expected-warning {{contract assertion cannot be proven}}
  static A b2{DP(), nothr()}; // expected-warning {{contract assertion cannot be proven}}
  A b3(DP(), 1); // expected-warning {{contract assertion cannot be proven}}
  D b4{{}, 1};
  static AI b5{DP()}; // expected-warning {{contract assertion cannot be proven}}
  A b6{DP()};
  L b7{1, DP()};
  auto b8 = [p = DP(), i = 1] {}; // expected-warning 2 {{contract assertion cannot be proven}}
  static DN b9[2] = {DN(), DN()}; // expected-warning 2 {{contract assertion cannot be proven}} expected-note {{'b9', of static storage duration, is destroyed by a call through the address of its destructor}}
  // A capture that comes last: only the closure's destructor, no partial
  // cleanup of p.
  auto b11 = [i = 1, p = DP()] // expected-warning {{contract assertion cannot be proven}}
    {};
  (void)b1;
  (void)b2;
  (void)b3;
  (void)b4;
  (void)b5;
  (void)b6;
  (void)b7;
  (void)b8;
  (void)b11;
} // expected-warning 2 {{contract assertion cannot be proven}}

void h2() {
  AR b10{{DN(), DN()}, 1};
  (void)b10;
} // expected-warning {{contract assertion cannot be proven}}

// An array member with no later member: a[1] follows a[0], so a[0]'s
// cleanup is still potentially invoked; diagnosed at the end of the
// function, which `h3' has to itself, as `h2' for `b10'.
void h3() {
  AR b12{{DN(), DN()}};
  (void)b12;
} // expected-warning {{contract assertion cannot be proven}}
