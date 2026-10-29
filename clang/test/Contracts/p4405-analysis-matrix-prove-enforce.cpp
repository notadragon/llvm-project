// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -ferror-limit=0 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Adjusting to the Allowed Semantics", "P3400R5 Analysis
// Evaluation Configuration"): the label x configuration matrix, configured by flags.
// Configured: analysis prove, runtime enforce.  Each label's assertions are
// a precondition (analyzed at each call: unguarded, guarded, and with a
// constant argument), an unprovable and a provable postcondition, and an
// unguarded and a guarded contract_assert.  The analysis semantic follows
// the label: none, symbolic and an analysis_semantic facet take the
// configured one (a facet the stronger of the two), always_prove and
// always_diagnose force theirs, terminating keeps only prove, always_enforce
// is never analyzed, a combined label takes the stronger facet adjusted to
// the combined allowed set, and a compute_semantic result of prove or
// diagnose moves to the analysis evaluation.  The runtime configuration
// never changes what is diagnosed.  One file of the matrix
// (p4405-analysis-matrix-*.cpp); optimized, so the proof shadow proves the
// guarded cases.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-matrix-prove-enforce.C

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set allowed_semantics{prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set allowed_semantics{diagnose};
};
constexpr always_diagnose_t always_diagnose{};

template <evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr evaluation_semantic analysis_semantic() const { return S; }
};
constexpr analyzed<ignore> unanalyzed{};
constexpr analyzed<diagnose> diagnosed{};
constexpr analyzed<prove> proven{};

template <evaluation_semantic S>
struct computes {
  using assertion_control_object = computes;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const
  { return S; }
};
constexpr computes<prove> computes_prove{};
constexpr computes<diagnose> computes_diagnose{};

// no label: analysis prove.
int pre_none(int x) pre(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_none(int x) post(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_none(int x) post(r: r > 0) { return x > 0 ? x : 1; }
int assert_none(int x)
{
  contract_assert(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert(x > 0);
  return x;
}
void calls_none(int y)
{
  pre_none(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_none(y);
  pre_none(1);
}

// always_prove: analysis prove.
int pre_ap(int x) pre<always_prove>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_ap(int x) post<always_prove>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_ap(int x) post<always_prove>(r: r > 0) { return x > 0 ? x : 1; }
int assert_ap(int x)
{
  contract_assert<always_prove>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<always_prove>(x > 0);
  return x;
}
void calls_ap(int y)
{
  pre_ap(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_ap(y);
  pre_ap(1);
}

// always_diagnose: analysis diagnose.
int pre_ad(int x) pre<always_diagnose>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_ad(int x) post<always_diagnose>(r: r > 0) { return x; }  // expected-warning {{contract assertion cannot be proven}}
int postok_ad(int x) post<always_diagnose>(r: r > 0) { return x > 0 ? x : 1; }
int assert_ad(int x)
{
  contract_assert<always_diagnose>(x > 0);  // expected-warning {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<always_diagnose>(x > 0);
  return x;
}
void calls_ad(int y)
{
  pre_ad(y);  // expected-warning {{contract assertion cannot be proven}}
  if (y > 0)
    pre_ad(y);
  pre_ad(1);
}

// L::terminating: analysis prove.
int pre_term(int x) pre<L::terminating>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_term(int x) post<L::terminating>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_term(int x) post<L::terminating>(r: r > 0) { return x > 0 ? x : 1; }
int assert_term(int x)
{
  contract_assert<L::terminating>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<L::terminating>(x > 0);
  return x;
}
void calls_term(int y)
{
  pre_term(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_term(y);
  pre_term(1);
}

// L::symbolic: analysis prove.
int pre_sym(int x) pre<L::symbolic>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_sym(int x) post<L::symbolic>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_sym(int x) post<L::symbolic>(r: r > 0) { return x > 0 ? x : 1; }
int assert_sym(int x)
{
  contract_assert<L::symbolic>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<L::symbolic>(x > 0);
  return x;
}
void calls_sym(int y)
{
  pre_sym(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_sym(y);
  pre_sym(1);
}

// L::always_enforce: analysis ignore.
int pre_ae(int x) pre<L::always_enforce>(x > 0) { return x; }
int post_ae(int x) post<L::always_enforce>(r: r > 0) { return x; }
int postok_ae(int x) post<L::always_enforce>(r: r > 0) { return x > 0 ? x : 1; }
int assert_ae(int x)
{
  contract_assert<L::always_enforce>(x > 0);
  if (x > 0)
    contract_assert<L::always_enforce>(x > 0);
  return x;
}
void calls_ae(int y)
{
  pre_ae(y);
  if (y > 0)
    pre_ae(y);
  pre_ae(1);
}

// diagnosed: analysis prove.
int pre_fd(int x) pre<diagnosed>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_fd(int x) post<diagnosed>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_fd(int x) post<diagnosed>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fd(int x)
{
  contract_assert<diagnosed>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<diagnosed>(x > 0);
  return x;
}
void calls_fd(int y)
{
  pre_fd(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_fd(y);
  pre_fd(1);
}

// proven: analysis prove.
int pre_fp(int x) pre<proven>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_fp(int x) post<proven>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_fp(int x) post<proven>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fp(int x)
{
  contract_assert<proven>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<proven>(x > 0);
  return x;
}
void calls_fp(int y)
{
  pre_fp(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_fp(y);
  pre_fp(1);
}

// diagnosed | proven: analysis prove.
int pre_fdfp(int x) pre<diagnosed | proven>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_fdfp(int x) post<diagnosed | proven>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_fdfp(int x) post<diagnosed | proven>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fdfp(int x)
{
  contract_assert<diagnosed | proven>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<diagnosed | proven>(x > 0);
  return x;
}
void calls_fdfp(int y)
{
  pre_fdfp(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_fdfp(y);
  pre_fdfp(1);
}

// unanalyzed | diagnosed: analysis prove.
int pre_fudi(int x) pre<unanalyzed | diagnosed>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_fudi(int x) post<unanalyzed | diagnosed>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_fudi(int x) post<unanalyzed | diagnosed>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fudi(int x)
{
  contract_assert<unanalyzed | diagnosed>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<unanalyzed | diagnosed>(x > 0);
  return x;
}
void calls_fudi(int y)
{
  pre_fudi(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_fudi(y);
  pre_fudi(1);
}

// diagnosed | L::symbolic: analysis prove.
int pre_fdsym(int x) pre<diagnosed | L::symbolic>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_fdsym(int x) post<diagnosed | L::symbolic>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_fdsym(int x) post<diagnosed | L::symbolic>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fdsym(int x)
{
  contract_assert<diagnosed | L::symbolic>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<diagnosed | L::symbolic>(x > 0);
  return x;
}
void calls_fdsym(int y)
{
  pre_fdsym(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_fdsym(y);
  pre_fdsym(1);
}

// diagnosed | L::terminating: analysis prove.
int pre_fdterm(int x) pre<diagnosed | L::terminating>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_fdterm(int x) post<diagnosed | L::terminating>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_fdterm(int x) post<diagnosed | L::terminating>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fdterm(int x)
{
  contract_assert<diagnosed | L::terminating>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<diagnosed | L::terminating>(x > 0);
  return x;
}
void calls_fdterm(int y)
{
  pre_fdterm(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_fdterm(y);
  pre_fdterm(1);
}

// proven | L::always_enforce: analysis ignore.
int pre_fpae(int x) pre<proven | L::always_enforce>(x > 0) { return x; }
int post_fpae(int x) post<proven | L::always_enforce>(r: r > 0) { return x; }
int postok_fpae(int x) post<proven | L::always_enforce>(r: r > 0) { return x > 0 ? x : 1; }
int assert_fpae(int x)
{
  contract_assert<proven | L::always_enforce>(x > 0);
  if (x > 0)
    contract_assert<proven | L::always_enforce>(x > 0);
  return x;
}
void calls_fpae(int y)
{
  pre_fpae(y);
  if (y > 0)
    pre_fpae(y);
  pre_fpae(1);
}

// computes_prove: analysis prove.
int pre_cp(int x) pre<computes_prove>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_cp(int x) post<computes_prove>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_cp(int x) post<computes_prove>(r: r > 0) { return x > 0 ? x : 1; }
int assert_cp(int x)
{
  contract_assert<computes_prove>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<computes_prove>(x > 0);
  return x;
}
void calls_cp(int y)
{
  pre_cp(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_cp(y);
  pre_cp(1);
}

// computes_diagnose: analysis prove.
int pre_cd(int x) pre<computes_diagnose>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int post_cd(int x) post<computes_diagnose>(r: r > 0) { return x; }  // expected-error {{contract assertion cannot be proven}}
int postok_cd(int x) post<computes_diagnose>(r: r > 0) { return x > 0 ? x : 1; }
int assert_cd(int x)
{
  contract_assert<computes_diagnose>(x > 0);  // expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert<computes_diagnose>(x > 0);
  return x;
}
void calls_cd(int y)
{
  pre_cd(y);  // expected-error {{contract assertion cannot be proven}}
  if (y > 0)
    pre_cd(y);
  pre_cd(1);
}
