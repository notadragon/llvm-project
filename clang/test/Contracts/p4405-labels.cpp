// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=prove %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s

// P4405: assertion-control labels and the prove and diagnose semantics.  A
// label's allowed_semantics may contain or exclude them; a prove the label
// does not allow becomes diagnose, failing that ignore, and failing both
// the walk goes on to the safer levels.  (Other semantics reaching prove
// and diagnose: p4405-labels-reach.cpp and p4405-labels-always.cpp.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-labels.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

constexpr allow<prove, diagnose, ignore> with_both{};
constexpr allow<diagnose, ignore, enforce> no_prove{};
constexpr allow<ignore, enforce> neither{};
constexpr allow<enforce> enforce_only{};
constexpr allow<prove> prove_only{};

int a(int x) pre<with_both>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int b(int x) pre<no_prove>(x > 0) { return x; }     // expected-note {{the predicate of the contract assertion}}
int c(int x) pre<neither>(x > 0) { return x; }
int d(int x) pre<enforce_only>(x > 0) { return x; }
int e(int x) pre<prove_only>(x > 0) { return x; }   // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a(y); // expected-error {{contract assertion cannot be proven}}
  b(y); // expected-warning {{contract assertion cannot be proven}}
  c(y);
  d(y);
  e(y); // expected-error {{contract assertion cannot be proven}}
}

// The labels' sets themselves.
static_assert(decltype(with_both)::allowed_semantics.contains(prove));
static_assert(decltype(with_both)::allowed_semantics.contains(diagnose));
static_assert(!decltype(neither)::allowed_semantics.contains(prove));
static_assert(!decltype(neither)::allowed_semantics.contains(diagnose));

// REQUIRES: contracts-libcxx
