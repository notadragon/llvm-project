// REQUIRES: contracts-libcxx
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s

// P4405: adjusting a semantic to a label's allowed set reaches prove and
// diagnose.  Both sit at ignore's level, where ignore is preferred, then
// diagnose, then prove (D3595R1, "Adjusting a Semantic to the Allowed
// Set").  An enforce contract whose label allows none of the semantics at
// levels 3 and 4 (enforce, quick_enforce) or at level 2 (observe) walks
// down to ignore's level and takes the first of ignore, diagnose and prove
// the label allows; an unprovable predicate shows which.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-labels-reach.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

constexpr allow<prove, diagnose> proving_only{};
constexpr allow<prove> prove_only{};
constexpr allow<diagnose> diagnose_only{};
constexpr allow<ignore, prove> ignore_or_prove{};
constexpr allow<enforce, prove> enforce_or_prove{};
constexpr allow<observe, diagnose> observe_or_diagnose{};

// Diagnose is preferred to prove.
int a(int x) pre<proving_only>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int b(int x) pre<prove_only>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int c() pre<prove_only>(true) { return 0; }
int d(int x) pre<diagnose_only>(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}

// Ignore is preferred to prove.
int e(int x) pre<ignore_or_prove>(x > 0) { return x; }

// An allowed enforce is kept, and observe, at a nearer level, is preferred
// to diagnose.
int f(int x) pre<enforce_or_prove>(x > 0) { return x; }
int g(int x) pre<observe_or_diagnose>(x > 0) { return x; }

void ae2_calls(int y) {
  a(y); // expected-warning {{contract assertion cannot be proven}}
  b(y); // expected-error {{contract assertion cannot be proven}}
  c();
  d(y); // expected-warning {{contract assertion cannot be proven}}
  e(y);
  f(y);
  g(y);
}
