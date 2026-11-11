// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=assume %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3100 x P3400 in constant evaluation, without -fcontracts-allow-assume: the
// configured "assume" is ignore, so a label allowing only assume leaves no
// evaluation semantic, a compute_semantic result of assume is not allowed,
// and a label that maps assume to enforce sees ignore (and gives observe).
// The run-time shape is in Runnable/p3100-label-no-flag.cpp.  (From the EDG
// fork's p3100/assume_labels_no_option.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-label-assume-constexpr-no-flag.C,
// where the first error reads "no valid evaluation semantic".

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// Only assume: assume itself.
struct only_assume_t {
  using assertion_control_object = only_assume_t;
  static constexpr evaluation_semantic_set allowed_semantics = {
      evaluation_semantic::assume};
};
// Only observe: neither assume nor ignore, so up to observe.
struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic_set allowed_semantics = {
      evaluation_semantic::observe};
};
// ignore and enforce: ignore, the nearest level up from assume.
struct ignore_enforce_t {
  using assertion_control_object = ignore_enforce_t;
  static constexpr evaluation_semantic_set allowed_semantics = {
      evaluation_semantic::ignore, evaluation_semantic::enforce};
};
// Computes assume from anything.
struct compute_assume_t {
  using assertion_control_object = compute_assume_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::assume;
  }
};
// Computes enforce from assume, observe from anything else.
struct enforce_from_assume_t {
  using assertion_control_object = enforce_from_assume_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s == evaluation_semantic::assume ? evaluation_semantic::enforce
                                            : evaluation_semantic::observe;
  }
};

constexpr int a(int x) pre<only_assume_t{}>(x > 0) { return x; } // expected-error {{assertion-control label of type 'only_assume_t' allows no evaluation semantics}}
constexpr int b(int x) pre<only_observe_t{}>(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int c(int x) pre<ignore_enforce_t{}>(x > 0) { return x; }
constexpr int d(int x) pre<compute_assume_t{}>(x > 0) { return x; } // expected-error {{compute_semantic result is not in the allowed evaluation semantics}}
constexpr int e(int x) pre<enforce_from_assume_t{}>(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}

constexpr int vb = b(0); // observe: a warning at the assertion
constexpr int vc = c(0); // OK, ignore
constexpr int ve = e(0); // observe (computed from ignore)

void use() {
  a(0);
  d(0);
}
