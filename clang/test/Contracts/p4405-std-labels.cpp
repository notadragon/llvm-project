// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Local Proofs", "Controlling the Analysis Semantic with Labels"):
// <contracts> declares the standard labels prove, diagnose, always_prove
// and always_diagnose in std::contracts::labels with -fcontracts-p4405.
// prove and diagnose are an analysis_semantic facet and nothing else, so
// the runtime evaluations keep the configured semantic; always_prove and
// always_diagnose allow only prove (diagnose).  Combined with |, the
// combined facet is the stronger analysis semantic and the allowed sets
// intersect.  Each label decides the analysis evaluation of a precondition
// (at each call), a postcondition and a contract_assert.  Configured
// analysis: ignore (the default), so only the labels analyze.
// (p4405-std-labels-without-flag.cpp: absent without -fcontracts-p4405.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-std-labels.C

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// The types and their facets.
static_assert(__is_same(decltype(L::prove), const L::prove_t));
static_assert(__is_same(decltype(L::diagnose), const L::diagnose_t));
static_assert(__is_same(decltype(L::always_prove), const L::always_prove_t));
static_assert(__is_same(decltype(L::always_diagnose),
                        const L::always_diagnose_t));
static_assert(__is_same(L::prove_t::assertion_control_object, L::prove_t));
static_assert(__is_same(L::diagnose_t::assertion_control_object,
                        L::diagnose_t));
static_assert(__is_same(L::always_prove_t::assertion_control_object,
                        L::always_prove_t));
static_assert(__is_same(L::always_diagnose_t::assertion_control_object,
                        L::always_diagnose_t));

static_assert(L::analysis_semantic_label<L::prove_t>);
static_assert(L::analysis_semantic_label<L::diagnose_t>);
static_assert(!L::allowed_semantics_label<L::prove_t>);
static_assert(!L::allowed_semantics_label<L::diagnose_t>);
static_assert(!L::semantic_computation_label<L::prove_t>);
static_assert(!L::semantic_computation_label<L::diagnose_t>);
static_assert(L::prove.analysis_semantic() == evaluation_semantic::prove);
static_assert(L::diagnose.analysis_semantic() ==
              evaluation_semantic::diagnose);

static_assert(L::allowed_semantics_label<L::always_prove_t>);
static_assert(L::allowed_semantics_label<L::always_diagnose_t>);
static_assert(!L::analysis_semantic_label<L::always_prove_t>);
static_assert(!L::analysis_semantic_label<L::always_diagnose_t>);
static_assert(!L::semantic_computation_label<L::always_prove_t>);
static_assert(L::always_prove.allowed_semantics ==
              evaluation_semantic_set{evaluation_semantic::prove});
static_assert(L::always_diagnose.allowed_semantics ==
              evaluation_semantic_set{evaluation_semantic::diagnose});

// Combinations: the stronger facet, the intersection of the allowed sets.
static_assert((L::prove | L::diagnose).analysis_semantic() ==
              evaluation_semantic::prove);
static_assert((L::diagnose | L::prove).analysis_semantic() ==
              evaluation_semantic::prove);
static_assert((L::diagnose | L::always_enforce).analysis_semantic() ==
              evaluation_semantic::diagnose);
static_assert((L::diagnose | L::always_enforce).allowed_semantics ==
              evaluation_semantic_set{evaluation_semantic::enforce});
static_assert((L::always_prove | L::symbolic).allowed_semantics ==
              evaluation_semantic_set{evaluation_semantic::prove});
static_assert((L::always_prove | L::always_diagnose).allowed_semantics.empty());
static_assert(!L::analysis_semantic_label<
              decltype(L::always_prove | L::always_diagnose)>);

// The variable template for the concept, applied to the labels themselves
// and to their combinations, which are of structural types.
static_assert(L::is_analysis_semantic_label<L::prove>);
static_assert(L::is_analysis_semantic_label<L::diagnose>);
static_assert(L::is_analysis_semantic_label<L::prove | L::diagnose>);
static_assert(L::is_analysis_semantic_label<L::diagnose | L::review>);
static_assert(L::is_analysis_semantic_label<L::diagnose | L::always_enforce>);
static_assert(L::is_analysis_semantic_label<L::terminating | L::prove>);
static_assert(L::is_analysis_semantic_label<L::always_prove | L::diagnose>);
static_assert(!L::is_analysis_semantic_label<L::always_prove |
                                             L::always_diagnose>);
static_assert(!L::is_analysis_semantic_label<L::always_prove>);
static_assert(!L::is_analysis_semantic_label<L::always_diagnose>);
static_assert(!L::is_analysis_semantic_label<L::empty_label>);
static_assert(!L::is_analysis_semantic_label<L::terminating>);

// The labels on contract assertions, named unqualified (the
// contract_control lookup <contracts> installs) and qualified.
void pp(int x) pre<prove>(x > 0);            // expected-note {{the predicate of the contract assertion}}
void pd(int x) pre<diagnose>(x > 0);         // expected-note {{the predicate of the contract assertion}}
void pap(int x) pre<always_prove>(x > 0);    // expected-note {{the predicate of the contract assertion}}
void pad(int x) pre<always_diagnose>(x > 0); // expected-note {{the predicate of the contract assertion}}
void ppd(int x) pre<prove | diagnose>(x > 0); // expected-note {{the predicate of the contract assertion}}
void pdq(int x) pre<L::diagnose>(x > 0);     // expected-note {{the predicate of the contract assertion}}

void calls(int y) {
  pp(y);  // expected-error {{contract assertion cannot be proven}}
  pd(y);  // expected-warning {{contract assertion cannot be proven}}
  pap(y); // expected-error {{contract assertion cannot be proven}}
  pad(y); // expected-warning {{contract assertion cannot be proven}}
  ppd(y); // expected-error {{contract assertion cannot be proven}}
  pdq(y); // expected-warning {{contract assertion cannot be proven}}
}

int qp(int x) post<prove>(r : r > 0) { return x; }            // expected-error {{contract assertion cannot be proven}}
int qd(int x) post<diagnose>(r : r > 0) { return x; }         // expected-warning {{contract assertion cannot be proven}}
int qap(int x) post<always_prove>(r : r > 0) { return x; }    // expected-error {{contract assertion cannot be proven}}
int qad(int x) post<always_diagnose>(r : r > 0) { return x; } // expected-warning {{contract assertion cannot be proven}}

void asserts(int x) {
  contract_assert<prove>(x > 0);           // expected-error {{contract assertion cannot be proven}}
  contract_assert<diagnose>(x > 0);        // expected-warning {{contract assertion cannot be proven}}
  contract_assert<always_prove>(x > 0);    // expected-error {{contract assertion cannot be proven}}
  contract_assert<always_diagnose>(x > 0); // expected-warning {{contract assertion cannot be proven}}
}

// Provable: nothing is diagnosed.
void proven(int x) {
  contract_assert<prove>(sizeof(x) == sizeof(int));
  contract_assert<always_prove>(true);
  contract_assert<diagnose>(1 + 1 == 2);
}

// E198: diagnose combined with a label that restricts the runtime
// semantics is adjusted to the combined allowed set -- {enforce}, or
// terminating's {enforce, quick_enforce, prove} -- where diagnose is not,
// so the analysis evaluation falls back to ignore and nothing is
// diagnosed.  (Diagnose-then-enforce is diagnose | L with L a
// compute_semantic label: p4405-std-labels-diagnose-enforced-*-run.cpp.)
void ae(int x) pre<diagnose | always_enforce>(x > 0);
void at(int x) pre<diagnose | terminating>(x > 0);

void restricted(int y) {
  ae(y);
  at(y);
  contract_assert<diagnose | always_enforce>(y > 0);
  contract_assert<diagnose | terminating>(y > 0);
}
