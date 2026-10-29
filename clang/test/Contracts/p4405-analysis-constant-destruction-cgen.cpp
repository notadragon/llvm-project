// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0 ("The Analysis Evaluation"; DECISIONS.md E142): there is no
// analysis evaluation during constant evaluation.  A constexpr or constinit
// variable's initialization is manifestly constant-evaluated, and so is the
// destructor call of a constexpr variable's constant destruction: none of
// those calls has an analysis evaluation, so a precondition that holds in
// constant evaluation (__builtin_is_constant_evaluated()) and that folding
// cannot prove outside it is not diagnosed -- at namespace scope, an array,
// a member, a static member, a variable template, a function's automatic
// and static ones, a template's.  Code generation, not -fsyntax-only (gnu_gcc
// diagnosed the temporaries of such an initialization and the constant
// destruction at code generation only, GCC-943; Clang makes its analysis
// evaluations in code generation, which sees none of these calls).
//
// Still diagnosed: an automatic object destroyed at run time (`DE d'),
// where its destructor is called, once; and the call of `~DE' in the body of
// H's implicit destructor, once, at the class.  Clang analyzes a function's
// calls only where it emits the function (E130 (a), the Divergences row
// CLANG-820), and constant destruction of `ch' emits no `~H', so `hh'
// destroys an automatic H; gnu_gcc diagnoses `~H' for `ch' alone.
//
// Divergence (DECISIONS.md E149 (a), CLANG-865): gnu_gcc gives `constinit
// DE ci' and `DE e1' the address-of error, once each; Clang does not destroy
// a non-constexpr object whose destruction is constant, so neither is
// diagnosed here.  The same property -- an object of static storage duration
// destroyed through its destructor's address is diagnosed once, and its
// registration with no analysis of its own -- is shown by `DF', whose
// precondition is not a core constant expression, under always_diagnose (an
// address-of error would stop code generation): `f1' and the constinit `f2'
// get the address-of warning once each, and K's implicit destructor, emitted
// for `k1', its call of `~DF' once, at the class.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-constant-destruction-cgen.C

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

struct DE {
  constexpr ~DE() pre<always_prove>(__builtin_is_constant_evaluated() || g > 0) {} // expected-note 2 {{the predicate of the contract assertion}}
};
struct H { // expected-error {{contract assertion cannot be proven}}
  DE d;
};

constexpr DE ce1;
constexpr DE ce2{};
constexpr DE ce3 = DE();
constexpr DE ca[2];
constexpr H ch;
constexpr const DE &cr = ce1;

constinit DE ci;
DE e1;

struct S {
  static constexpr DE sm{};
};

template <class T> constexpr T vt{};
template <class T> void tf() {
  constexpr T a;
  static constexpr T s;
}

void f() {
  constexpr DE a;
  constexpr DE b{};
  constexpr DE c[2];
  static constexpr DE s;
  (void)&vt<DE>;
  (void)&S::sm;
  tf<DE>();
}

void h() {
  DE d; // expected-error {{contract assertion cannot be proven}}
}

void hh() { H h2; }

struct DF {
  constexpr ~DF() pre<always_diagnose>(g > 0) {} // expected-note 2 {{precondition here}} expected-note {{the predicate of the contract assertion}}
};
DF f1; // expected-warning {{contract assertion cannot be proven for a call through the address of '~DF'}} expected-note {{'f1', of static storage duration, is destroyed by a call through the address of its destructor}}
constinit DF f2; // expected-warning {{contract assertion cannot be proven for a call through the address of '~DF'}} expected-note {{'f2', of static storage duration, is destroyed by a call through the address of its destructor}}
struct K { // expected-warning {{contract assertion cannot be proven}}
  DF d;
};
K k1;
