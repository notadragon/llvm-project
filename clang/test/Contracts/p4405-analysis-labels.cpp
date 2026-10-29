// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s

// D4405 ("P3400R5 Analysis Evaluation Configuration", "Adjusting to the
// Allowed Semantics"): resolving the analysis evaluation under labels, with
// the runtime evaluations configured ignore and no analysis configured.  An
// analysis_semantic facet strengthens the analysis evaluation (step 3); a
// combined label gives the stronger of its constituents'; the result is
// adjusted to the analysis allowed set, the label's prove and diagnose plus
// ignore when it allows a runtime semantic (step 4).  An unprovable
// predicate shows which semantic the analysis evaluation got.
//
// The analysis evaluation of a precondition is made at each call, in the
// caller, so the precondition is diagnosed at the call (ae2_calls), with a
// note at its predicate; a postcondition's and an assertion's are made in
// the function body.
//
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-labels.C

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr evaluation_semantic analysis_semantic() const { return S; }
};
constexpr analyzed<ignore> unanalyzed{};
constexpr analyzed<diagnose> diagnosed{};
constexpr analyzed<prove> proven{};

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};
constexpr allow<prove> prove_only{};

// Step 3.
int a1(int x) pre<unanalyzed>(x > 0) { return x; }
int a2(int x) pre<diagnosed>(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}
int a3(int x) pre<proven>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int a4(int x) {
  contract_assert<diagnosed>(x > 0); // expected-warning {{contract assertion cannot be proven}}
  return x;
}
int a5(int x) post<proven>(r: r > 0) { return x; } // expected-error {{contract assertion cannot be proven}}

// Combined labels: the stronger.
int b1(int x) pre<diagnosed | proven>(x > 0) { return x; }        // expected-note {{the predicate of the contract assertion}}
int b2(int x) pre<unanalyzed | diagnosed>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int b3(int x) pre<L::empty_label | diagnosed>(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}

// Step 4: the label's allowed set.  symbolic allows diagnose and prove;
// prove_only has no runtime semantic, so no ignore for the analysis
// evaluation, and diagnose falls back to prove; always_enforce has no
// diagnose or prove, so its assertions are never analyzed.
int c1(int x) pre<L::symbolic>(x > 0) { return x; }
int c2(int x) pre<L::symbolic | diagnosed>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int c3(int x) pre<L::symbolic | proven>(x > 0) { return x; }     // expected-note {{the predicate of the contract assertion}}
int c4(int x) pre<prove_only | diagnosed>(x > 0) { return x; }   // expected-note {{the predicate of the contract assertion}}
int c5(int x) pre<L::always_enforce | proven>(x > 0) { return x; }
int c7(int x) pre<prove_only>(x > 0) { return x; }               // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a1(y);
  a2(y); // expected-warning {{contract assertion cannot be proven}}
  a3(y); // expected-error {{contract assertion cannot be proven}}
  b1(y); // expected-error {{contract assertion cannot be proven}}
  b2(y); // expected-warning {{contract assertion cannot be proven}}
  b3(y); // expected-warning {{contract assertion cannot be proven}}
  c1(y);
  c2(y); // expected-warning {{contract assertion cannot be proven}}
  c3(y); // expected-error {{contract assertion cannot be proven}}
  c4(y); // expected-error {{contract assertion cannot be proven}}
  c5(y);
  c7(y); // expected-error {{contract assertion cannot be proven}}
}

// REQUIRES: contracts-libcxx
