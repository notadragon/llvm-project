// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o /dev/null -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Virtual Functions": each override obligation is an analysis
// evaluation of the overrider's precondition, with that precondition's
// own analysis semantic: here the build's is ignore, so only a
// precondition labeled always_prove is analyzed in the override shadow.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-labels.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic S>
struct always {
  using assertion_control_object = always;
  static constexpr evaluation_semantic_set allowed_semantics{S};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return S;
  }
};
constexpr always<prove> always_prove{};

struct B {
  virtual int f(int x) pre(x > 0);
  virtual int g(int x) pre(x > 0);
};

struct D : B {
  int f(int x) override         // expected-error {{cannot be proven for a call through 'B::f'}} expected-remark {{override shadow: not proven}}
    pre<always_prove>(x > 1)    // expected-note {{the predicate of the contract assertion}}
    pre(x > 2)                  // not analyzed: ignore
  { return x; }
  int g(int x) override         // expected-remark {{override shadow: proven, conditional}}
    pre<always_prove>(x >= 1)
  { return x; }
};

void use() { D d; }
