// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3100 x P3400 in constant evaluation, with -fcontracts-allow-assume: the
// configured "assume" is intersected with each label's allowed set, falling
// to ignore and then upwards; a compute_semantic result of assume is allowed,
// and one that maps assume to enforce is used.  The run-time shapes are in
// Runnable/p3100-label-assume.cpp.  (From the EDG fork's p3100/assume_labels.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-label-assume-constexpr.C

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

constexpr int a(int x) pre<only_assume_t{}>(x > 0) { return x; }
constexpr int b(int x) pre<only_observe_t{}>(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int c(int x) pre<ignore_enforce_t{}>(x > 0) { return x; }
constexpr int d(int x) pre<compute_assume_t{}>(x > 0) { return x; }
constexpr int e(int x) pre<enforce_from_assume_t{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}

constexpr int va = a(0); // OK, assume: not evaluated
constexpr int vb = b(0); // observe: a warning at the assertion
constexpr int vc = c(0); // OK, ignore
constexpr int vd = d(0); // OK, assume
constexpr int ve = e(0); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
