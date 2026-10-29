// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): the destructor call of a temporary that
// an initialization elides is never made, so it has no analysis evaluation:
// only a temporary that is materialized and destroyed has one.  At
// namespace scope and in a function, without optimization (folding) and
// with it (the proof shadow), where nothing can prove `g > 0'.  The
// variables of static storage duration still get their address-of
// diagnostic (D4405R0, "Taking the Address of a Function"); an automatic
// variable's own destruction is analyzed at the end of its scope, so that
// case is elided into an object nobody destroys (`new') or into a return
// value.  Code generation, not -fsyntax-only.
//
// gnu_gcc's tests use always_prove; here the precondition is
// always_diagnose, because Clang's address-of diagnostic is made in Sema
// and as an error would stop code generation, where the analysis
// evaluations are made.  So every diagnostic is a -Wcontract-unproven
// warning.  Clang never analyzed an elided temporary's destructor call
// (gnu_gcc did, GCC-944): it emits the cleanup only for a materialized
// temporary.
//
// Divergence (DECISIONS.md E130 (a), the Divergences row CLANG-820): Clang
// analyzes a function's calls only where it emits the function, so the
// materialized temporary in the unused inline function `unused_kept' is not
// diagnosed; gnu_gcc diagnoses it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-elided-temporary.C
// (-O0) and p4405-analysis-elided-temporary-shadow.C (-O2)

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
  ~DP() pre<always_diagnose>(g > 0); // expected-note 3 {{precondition here}} expected-note 7 {{the predicate of the contract assertion}}
};
DP make();
void take(DP);

// Namespace scope: the static initialization function makes the calls.
DP x = // expected-warning {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'x', of static storage duration}}
  DP();
DP y = // expected-warning {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'y', of static storage duration}}
  make();
DP w = // expected-warning {{contract assertion cannot be proven for a call through the address of '~DP'}} expected-note {{'w', of static storage duration}}
  g ? DP() : make();
DP *np = new DP(DP());
DP *mp = new DP(make());
int z =
  (DP(), 1); // expected-warning {{contract assertion cannot be proven}}
int t =
  (take(DP()), 1); // expected-warning {{contract assertion cannot be proven}}

// In a function.
DP ret() {
  return DP();
}

DP ret_make() {
  return make();
}

DP ret_cond() {
  return g ? DP() : make();
}

void f() {
  DP *p = new DP(DP());
  DP *q = new DP(make());
  int a = (DP(), 1); // expected-warning {{contract assertion cannot be proven}}
  take(DP()); // expected-warning {{contract assertion cannot be proven}}
  take(make()); // expected-warning {{contract assertion cannot be proven}}
}

// An inline function nobody calls: not emitted, so not analyzed (E130).
inline DP unused_ret() {
  return DP();
}

inline int unused_kept() {
  return (DP(), 1);
}

// A template, in its instantiation.
template <typename T> T tret() {
  return T();
}

template <typename T> int tkept() {
  return (T(), 1); // expected-warning {{contract assertion cannot be proven}}
}

void h() {
  delete new DP(tret<DP>()); // expected-warning {{contract assertion cannot be proven}}
  tkept<DP>(); // expected-note {{in instantiation of function template specialization 'tkept<DP>' requested here}}
}
