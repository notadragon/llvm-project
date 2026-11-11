// DEFINE: %{cc} = %clangxx -std=c++26 -fcontracts-p3097 \
// DEFINE:   -fcontract-evaluation-semantic=ignore \
// DEFINE:   -fcontract-configuration-file=%S/Runnable/p4451-callerside-virtual.json \
// DEFINE:   %libcxx_flags -fsyntax-only -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// RUN: %{cc} -fcontracts-p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451
// REQUIRES: contracts-libcxx

// P3097, notadragon_wg21 DECISIONS.md E180 (5), in constant evaluation: the
// interface checks of a virtual call never add ignore to their allowed sets,
// so with ignore configured everywhere -- also for caller-side checks
// (Runnable/p4451-callerside-virtual.json) -- B::f's terminating
// precondition and postcondition still fail the evaluation of a call through
// a B reference to D::f, and only the ordinary precondition is skipped.
// Under -fcontracts-p4451 alone, -fcontracts-p3400 alone (the P3400 label)
// and both.  The constant-evaluation half of
// Runnable/p4451-callerside-virtual.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-callerside-virtual.C
// (and its -p3400 and -both variants)

#include <contracts>  // `terminating`

struct B {
  constexpr virtual int f(int x) const
    pre<terminating>(x > 0)  // expected-error {{contract failed during execution of constexpr function}}
    pre(x > 1)
    post<terminating>(r : r < 10) { return x; }  // expected-error {{contract failed during execution of constexpr function}}
  constexpr virtual ~B() = default;
};

struct D : B {
  constexpr int f(int x) const override { return x; }
};

constexpr int call(int x) {
  D d;
  const B &b = d;
  return b.f(x);  // expected-note 2 {{in call to}}
}

constexpr int a = call(1);   // the ordinary precondition is skipped
constexpr int b = call(-1);  // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int c = call(20);  // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
