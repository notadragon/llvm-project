// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-constexpr-enforce.json %libcxx_flags -fsyntax-only -Xclang -verify -Xclang -verify-ignore-unexpected=note %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-constexpr-enforce.json %libcxx_flags -fsyntax-only -Xclang -verify -Xclang -verify-ignore-unexpected=note -Xclang -fexperimental-new-constant-interpreter %s

// D4405 (notadragon_wg21 DECISIONS.md E142): constant evaluation is never
// analyzed, and a prove it reaches is enforce there -- the one evaluation
// being made must satisfy the predicate, so a violation makes the
// expression not a constant -- and a diagnose is observe, a warning.
// However it is reached: from an entry without the "constexpr" key (here
// every evaluation is otherwise ignore, so nothing is analyzed), or from a
// label's allowed set (prove_only, diagnose_only, under ignore) or its
// compute_semantic (always_prove, under ignore).  Both constant evaluators.
//
// Only the constant evaluations are checked here (-fsyntax-only): the
// preconditions' analysis evaluations would be at the calls, and every
// call is in a constexpr variable's initializer, which is manifestly
// constant-evaluated, so has none (D4405R0: "During constant evaluation
// there is no analysis evaluation"), as in gnu_gcc's mirror.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-constexpr-enforce.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic S>
struct only {
  using assertion_control_object = only;
  static constexpr evaluation_semantic_set allowed_semantics{S};
};
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set allowed_semantics{prove};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return prove;
  }
};

constexpr int p(int x) pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int pa = p(1);
constexpr int pb = p(-1); // expected-error {{constexpr variable 'pb' must be initialized by a constant expression}}

namespace d {
constexpr int f(int x) pre(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int fb = f(-1);
static_assert(fb == -1);
} // namespace d

namespace lab {
constexpr int g(int x) pre<only<prove>{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int gb = g(-1); // expected-error {{constexpr variable 'gb' must be initialized by a constant expression}}
constexpr int h(int x) pre<only<diagnose>{}>(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int hb = h(-1);
static_assert(hb == -1);
constexpr int k(int x) pre<always_prove_t{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int kb = k(-1); // expected-error {{constexpr variable 'kb' must be initialized by a constant expression}}
} // namespace lab

// REQUIRES: contracts-libcxx
