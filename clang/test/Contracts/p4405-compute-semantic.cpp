// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce %libcxx_flags -DCODEGEN -S -emit-llvm -o /dev/null -Xclang -verify=codegen %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only -Xclang -verify %s

// P4405: a compute_semantic facet may return prove or diagnose.  Allowed,
// the evaluation is diagnosed accordingly; outside the label's allowed set
// it is an error, as for any other semantic, and so is a value that is no
// semantic at all.  (A Sema error stops code generation, which diagnoses
// prove and diagnose, so the two halves are compiled separately.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-compute-semantic.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template <evaluation_semantic R>
struct to {
  using assertion_control_object = to;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return R;
  }
};

template <evaluation_semantic R>
struct to_excluded {
  using assertion_control_object = to_excluded;
  static constexpr evaluation_semantic_set allowed_semantics{
      evaluation_semantic::enforce, evaluation_semantic::observe};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return R;
  }
};

constexpr to<evaluation_semantic::prove> to_prove{};
constexpr to<evaluation_semantic::diagnose> to_diagnose{};
constexpr to_excluded<evaluation_semantic::prove> to_prove_excluded{};
constexpr to_excluded<evaluation_semantic::diagnose> to_diagnose_excluded{};
constexpr to<(evaluation_semantic)10> to_unknown{};
constexpr to<(evaluation_semantic)0> to_unspecified{};

#ifdef CODEGEN
int a(int x) pre<to_prove>(x > 0) { return x; }     // codegen-note {{the predicate of the contract assertion}}
int b(int x) pre<to_diagnose>(x > 0) { return x; }  // codegen-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a(y); // codegen-error {{contract assertion cannot be proven}}
  b(y); // codegen-warning {{contract assertion cannot be proven}}
}
#else
int c(int x) pre<to_prove_excluded>(x > 0) { return x; }    // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}
int d(int x) pre<to_diagnose_excluded>(x > 0) { return x; } // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}
int e(int x) pre<to_unknown>(x > 0) { return x; }           // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}
int f(int x) pre<to_unspecified>(x > 0) { return x; }       // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}
#endif

// REQUIRES: contracts-libcxx
