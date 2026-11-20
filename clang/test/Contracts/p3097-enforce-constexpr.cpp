// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// P3097 x constant evaluation: under enforce, a violated assertion of a
// virtual call made during constant evaluation makes the program
// ill-formed -- the interface's precondition, checked by the wrapper, and
// the final overrider's own alike -- while the same calls outside constant
// evaluation are well-formed (they are checked, and terminate, at run time).
// The evaluation continues past a violation, so call(0) reports both
// preconditions (see constexpr-contract-every-problem.cpp).
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3097-enforce-constexpr.C

struct B {
  constexpr virtual int f(int x) const pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  constexpr virtual ~B() = default;
};

struct D : B {
  constexpr int f(int x) const override pre(x > 1) { return x; } // expected-error 2 {{contract failed during execution of constexpr function}}
};

constexpr int call(int x) {
  D d;
  const B &b = d;
  return b.f(x);
}

constexpr int ok = call(2);
// The interface precondition, then D's own.
constexpr int bad_iface = call(0); // expected-error {{must be initialized by a constant expression}}
constexpr int bad_own = call(1);   // expected-error {{must be initialized by a constant expression}}

int runtime(int x) {
  return call(x); // not constant evaluated: fine
}
