// REQUIRES: contracts-libcxx
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -ferror-limit=0 \
// RUN:   -fcontract-configuration-file=%S/p4405-labels-reach-configured.json \
// RUN:   %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s

// P4405 with P3400 labels: adjusting a semantic to a label's allowed set,
// when the walk reaches ignore's level, tries ignore, then diagnose, then
// prove before another level; a requested prove tries prove, diagnose,
// then ignore, and a requested diagnose tries diagnose, ignore, then prove
// (D3595R1, "Adjusting a Semantic to the Allowed Set").  Under D4405
// ("Adjusting to the Allowed Semantics") the walk reaching prove or
// diagnose is the analysis evaluation's, whose allowed set has ignore
// only when the label allows a runtime semantic, and a configured prove or
// diagnose moves to it; the runtime evaluations never reach prove or
// diagnose.  So a label allowing quick_enforce and prove always gives
// quick_enforce at run time, and the analysis evaluation prove only when
// prove is configured (diagnose falls back to ignore).  Each namespace is
// configured with one semantic; an unprovable predicate shows which
// semantic applied:
//
//                     ignore    quick_enforce  prove     diagnose
//   proving_only      diagnose  diagnose       prove     diagnose
//   prove_only        prove     prove          prove     prove
//   diagnose_only     diagnose  diagnose       diagnose  diagnose
//   ignore_or_prove   ignore    ignore         prove     ignore
//   qe_or_prove       quick     quick          prove+    quick
//   ignore_or_diag    ignore    ignore         diagnose  diagnose
//   always_prove      prove     prove          prove     prove
//
// (prove+: the analysis evaluation is prove, and the runtime evaluation
// quick_enforce; the analysis evaluation, made on its own at each call, is
// diagnosed whatever the runtime one.)  A precondition is analyzed at each
// call (ae2_calls), with a note at its predicate.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-labels-reach-configured.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set allowed_semantics{prove};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return prove;
  }
};
constexpr allow<prove, diagnose> proving_only{};
constexpr allow<prove> prove_only{};
constexpr allow<diagnose> diagnose_only{};
constexpr allow<ignore, prove> ignore_or_prove{};
constexpr allow<quick_enforce, prove> qe_or_prove{};
constexpr allow<ignore, diagnose> ignore_or_diag{};
constexpr always_prove_t always_prove{};

namespace c_ignore {
int a(int x) pre<proving_only>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int b(int x) pre<prove_only>(x > 0) { return x; }      // expected-note {{the predicate of the contract assertion}}
int c(int x) pre<diagnose_only>(x > 0) { return x; }   // expected-note {{the predicate of the contract assertion}}
int d(int x) pre<ignore_or_prove>(x > 0) { return x; }
int e(int x) pre<qe_or_prove>(x > 0) { return x; }
int f(int x) pre<ignore_or_diag>(x > 0) { return x; }
int g(int x) pre<always_prove>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a(y); // expected-warning {{contract assertion cannot be proven}}
  b(y); // expected-error {{contract assertion cannot be proven}}
  c(y); // expected-warning {{contract assertion cannot be proven}}
  g(y); // expected-error {{contract assertion cannot be proven}}
}
} // namespace c_ignore

namespace c_quick_enforce {
int a(int x) pre<proving_only>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int b(int x) pre<prove_only>(x > 0) { return x; }      // expected-note {{the predicate of the contract assertion}}
int c(int x) pre<diagnose_only>(x > 0) { return x; }   // expected-note {{the predicate of the contract assertion}}
int d(int x) pre<ignore_or_prove>(x > 0) { return x; }
int e(int x) pre<qe_or_prove>(x > 0) { return x; }
int f(int x) pre<ignore_or_diag>(x > 0) { return x; }
int g(int x) pre<always_prove>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a(y); // expected-warning {{contract assertion cannot be proven}}
  b(y); // expected-error {{contract assertion cannot be proven}}
  c(y); // expected-warning {{contract assertion cannot be proven}}
  g(y); // expected-error {{contract assertion cannot be proven}}
}
} // namespace c_quick_enforce

namespace c_prove {
int a(int x) pre<proving_only>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int b(int x) pre<prove_only>(x > 0) { return x; }      // expected-note {{the predicate of the contract assertion}}
int c(int x) pre<diagnose_only>(x > 0) { return x; }   // expected-note {{the predicate of the contract assertion}}
int d(int x) pre<ignore_or_prove>(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}
int e(int x) pre<qe_or_prove>(x > 0) { return x; }     // expected-note {{the predicate of the contract assertion}}
int f(int x) pre<ignore_or_diag>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int g(int x) pre<always_prove>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a(y); // expected-error {{contract assertion cannot be proven}}
  b(y); // expected-error {{contract assertion cannot be proven}}
  c(y); // expected-warning {{contract assertion cannot be proven}}
  d(y); // expected-error {{contract assertion cannot be proven}}
  e(y); // expected-error {{contract assertion cannot be proven}}
  f(y); // expected-warning {{contract assertion cannot be proven}}
  g(y); // expected-error {{contract assertion cannot be proven}}
}
} // namespace c_prove

namespace c_diagnose {
int a(int x) pre<proving_only>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}
int b(int x) pre<prove_only>(x > 0) { return x; }      // expected-note {{the predicate of the contract assertion}}
int c(int x) pre<diagnose_only>(x > 0) { return x; }   // expected-note {{the predicate of the contract assertion}}
int d(int x) pre<ignore_or_prove>(x > 0) { return x; }
int e(int x) pre<qe_or_prove>(x > 0) { return x; }
int f(int x) pre<ignore_or_diag>(x > 0) { return x; }  // expected-note {{the predicate of the contract assertion}}
int g(int x) pre<always_prove>(x > 0) { return x; }    // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  a(y); // expected-warning {{contract assertion cannot be proven}}
  b(y); // expected-error {{contract assertion cannot be proven}}
  c(y); // expected-warning {{contract assertion cannot be proven}}
  f(y); // expected-warning {{contract assertion cannot be proven}}
  g(y); // expected-error {{contract assertion cannot be proven}}
}
} // namespace c_diagnose
