// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o /dev/null -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Virtual Functions": an overridden function's postcondition is
// analyzed in an override shadow with its own analysis semantic: here the
// build's is ignore, so one labeled always_prove must be proven in every
// override, and the others are not analyzed (no override shadow for E).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-base-post-labels.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic S>
struct always {
  using assertion_control_object = always;
  static constexpr evaluation_semantic_set allowed_semantics{S};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const
  { return S; }
};
constexpr always<prove> always_prove{};

struct B {
  virtual int f(int x) post<always_prove>(r: r > 0);  // expected-note {{the predicate of the contract assertion}}
  virtual int g(int x) post(r: r > 0);
};

struct C : B {
  int f(int x) override { return x > 0 ? x : 1; }  // expected-remark {{override shadow: proven, sound}}
};

struct D : B {
  int f(int x) override         // expected-error {{cannot be proven for a call through 'B::f'}} expected-remark {{override shadow: not proven}}
  { return x; }
};

struct E : B {
  int g(int x) override { return x; }
};

void use() { C c; D d; E e; }
