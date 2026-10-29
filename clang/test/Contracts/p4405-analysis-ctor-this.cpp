// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -Wno-tautological-undefined-compare %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): a constructor or destructor
// precondition that names `this', analyzed at a call, is given the object
// the call constructs or destroys -- for a named object and for a prvalue's
// temporary alike, since Clang knows the temporary's address at the call --
// so a guarded call is proven, and an unguarded one is not.  An object's
// member read by a destructor's precondition is the object's.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-ctor-this.C
// (the elision forms are p4405-analysis-ctor-this-elide.cpp)

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct T {
  T(int y) pre<always_prove>(this != nullptr && y > 0); // expected-note {{the predicate of the contract assertion}}
  int v = 0;
};

struct U {
  int v;
  ~U() pre<always_prove>(v > 0); // expected-note {{the predicate of the contract assertion}}
};

int object(int y) {
  if (y > 0) {
    T t(y);
    return t.v;
  }
  return 0;
}

int prvalue(int y) {
  if (y > 0)
    return T(y).v;
  return 0;
}

int unguarded(int y) {
  return T(y).v; // expected-error {{contract assertion cannot be proven}}
}

void destroyed(int y) {
  if (y > 0) {
    U u{y};
  }
}

void destroyed_unguarded(int y) {
  U u{y}; // expected-error {{contract assertion cannot be proven}}
}
