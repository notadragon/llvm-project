// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): a precondition's analysis evaluation
// at a call of a constructor, or of a function returning a class, is
// deferred to the proof shadow like any other, so a guarded call is proven
// when optimizing (not only by folding), for a named object, a temporary
// (a prvalue of class type), a new-expression and a class with a virtual
// base (p4405-analysis-ctor-proof-noarg.cpp has calls with no other
// argument).  Unguarded calls still cannot be proven,
// and a constant argument is folded.  An unproven evaluation is a premise for
// those after it, so each unguarded call has its own variable.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-ctor-proof.C

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
  int v;
  T(int x) pre<always_prove>(x > 0) : v(x) {}  // expected-note {{the predicate of the contract assertion}}
  T(int x, const int &y) pre<always_prove>(x > y) : v(x - y) {}  // expected-note {{the predicate of the contract assertion}}
  ~T() {}
  T scaled(int k) const pre<always_prove>(k > 0) { T r = *this; r.v *= k; return r; }  // expected-note {{the predicate of the contract assertion}}
};

struct V : virtual T {
  V(int x) pre<always_prove>(x > 1) : T(x) {}  // expected-note {{the predicate of the contract assertion}}
};

struct R { int v; };
R mk(int x) pre<always_prove>(x > 0);  // expected-note {{the predicate of the contract assertion}}

int
guarded(int y, int z)
{
  int r = 0;
  if (y > 0)
    {
      T t(y);
      r += t.v;
    }
  if (y > 0)
    r += T(y).v;
  if (y > 0)
    r += (new T(y))->v;
  if (y > z)
    {
      T t(y, z);
      r += t.v;
    }
  if (y > 0)
    r += mk(y).v;
  if (y > 0)
    r += T(1).scaled(y).v;
  if (y > 1)
    {
      V w(y);
      r += w.v;
    }
  T c(1);
  return r + c.v;
}

int
unguarded(int a, int b, int c, int d, int e, int f)
{
  T t(a);  // expected-error {{contract assertion cannot be proven}}
  T u(b, c);  // expected-error {{contract assertion cannot be proven}}
  R r = mk(d);  // expected-error {{contract assertion cannot be proven}}
  V w(e);  // expected-error {{contract assertion cannot be proven}}
  return t.v + u.v + r.v + w.v + T(1).scaled(f).v;  // expected-error {{contract assertion cannot be proven}}
}
