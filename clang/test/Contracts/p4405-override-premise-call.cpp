// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove %libcxx_flags -O2 -S -emit-llvm -o /dev/null -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Previous Assertions and Undefined Behavior" and "Virtual
// Functions" (CLANG-918): an overridden function's precondition whose
// predicate calls a function is a premise in an override shadow, as a
// function's own precondition is in its proof shadow.  Each B precondition
// is always_enforce, so guaranteed (p4405-override-premise.cpp); the proof
// of D's precondition resting on it is sound if every function the
// predicate calls is found free of side effects, returning and not
// throwing (g: an inline function), and conditional otherwise (h: a
// function only declared here; k: an inline function holding a contract
// evaluation that is not settled).  Before CLANG-918 was fixed none of them
// was a premise, and each D precondition was "cannot be proven".
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-premise-call.C

#include <contracts>
using namespace std::contracts::labels;

bool opaque(int);
inline bool positive(int x) { return x > 0; }
inline bool checked(int x) pre(x != 5) { return x > 0; }

struct B {
  virtual int g(int x) pre<always_enforce>(positive(x));
  virtual int h(int x) pre<always_enforce>(opaque(x) && x > 0);
  virtual int k(int x) pre<always_enforce>(checked(x));
};

struct D : B {
  int g(int x) override pre(x >= 1) { return x; }  // expected-remark {{override shadow: proven, sound}}
  int h(int x) override pre(x >= 1) { return x; }  // expected-remark {{override shadow: proven, conditional}}
  int k(int x) override pre(x >= 1) { return x; }  // expected-remark {{override shadow: proven, conditional}}
};

void use() { D d; }
