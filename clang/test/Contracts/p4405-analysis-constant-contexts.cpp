// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify=expected,cg %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -fsyntax-only -Xclang -verify -Xclang -fexperimental-new-constant-interpreter %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): during constant evaluation there is no
// analysis evaluation, so a call in a manifestly constant-evaluated
// expression has none: the initializer of a constexpr or constinit
// variable (at namespace scope, in a function, a static data member), a
// static_assert condition, a template argument, an `if constexpr'
// condition, a case label, an enumerator, a bit-field width, and -- in a
// template -- each of those as it is instantiated; every call in an
// immediate function and in the consteval branch of `if consteval', and a
// call of an immediate function.  The constant evaluation evaluates the
// precondition, analyzed with diagnose, as observe: a warning at the
// precondition (DECISIONS.md E142), once or more for each constant
// evaluation.  A lambda's body defined in such an initializer is a function
// of its own, whose calls are analyzed, as is a run-time call.  Clang makes
// the analysis evaluations in code generation, which sees none of these
// calls; -verify rejects any analysis diagnostic not expected below.  The
// second RUN line checks the constant evaluations alone under the bytecode
// interpreter (-fsyntax-only: its folding of the run-time calls is
// CLANG-617's).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-constant-contexts.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

constexpr int h(int x) pre<always_diagnose>(x > 0) { return x; } // expected-warning 1+ {{contract failed during execution of constexpr function}} cg-note 3 {{the predicate of the contract assertion}}

template <int N> struct A { static constexpr int value = N; };

constexpr int v1 = h(-1);
constinit int v2 = h(-1);
static_assert(h(-1) == -1);
static_assert(A<h(-1)>::value == -1);
enum E { e = h(-1) };
struct B {
  static constexpr int s = h(-1);
  int bits : h(-1) + 3;
};

consteval int ce(int x) {
  return h(x);
}

constexpr int ci(int x) {
  if consteval {
    return h(x);
  }
  return 1;
}

int f(int y) {
  constexpr int l1 = h(-1);
  if constexpr (h(-1) < 0)
    y++;
  switch (y) {
  case h(-1):
    return l1;
  }
  y += ce(-1);
  y += ci(y);
  constexpr auto lam = [](int z) { return h(z); }; // cg-warning {{contract assertion cannot be proven}}
  y += lam(y);
  return y + h(-1); // cg-warning {{contract predicate is always false}}
}

template <typename T> int g(T y) {
  constexpr int l2 = h(-1);
  static_assert(h(-1) == -1);
  if constexpr (A<h(-1)>::value < 0)
    y++;
  return y + l2 + h(y); // cg-warning {{contract assertion cannot be proven}}
}

int gi = g(1); // cg-note {{in instantiation of function template specialization 'g<int>' requested here}}
