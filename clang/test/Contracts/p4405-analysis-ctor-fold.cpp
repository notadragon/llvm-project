// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=ignore -O2 -S -emit-llvm -o /dev/null -Xclang -verify %s

// D4405 ("The Analysis Evaluation"): a constexpr constructor call that
// initializes a variable and is folded to a constant -- possible when its
// precondition's constant evaluation is ignore -- still has its
// precondition's analysis evaluation where the call was: unguarded it
// cannot be proven, guarded it is (gnu_gcc's GCC-920 dropped the
// evaluation with the call; Clang emits these initializations as calls in
// any case).  The constant initialization of a
// variable of static storage duration is constant evaluation, which has no
// analysis evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-ctor-fold.C

extern int g;

struct T {
  constexpr T(int y) pre(g > 0) : v(y) {}  // expected-note {{the predicate of the contract assertion}}
  int v;
};

struct U {
  constexpr U() pre(g > 0) : v(1) {}  // expected-note {{the predicate of the contract assertion}}
  int v;
};

int h() { T t(1); return t.v; }  // expected-error {{contract assertion cannot be proven}}
int hu() { U u; return u.v; }  // expected-error {{contract assertion cannot be proven}}

int
k()
{
  if (g > 0)
    {
      T t(1);
      U u;
      return t.v + u.v;
    }
  return 0;
}

int
s()
{
  static T t(1);
  static U u;
  return t.v + u.v;
}
