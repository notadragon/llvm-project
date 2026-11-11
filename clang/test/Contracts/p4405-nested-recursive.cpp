// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0 %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169: the dependence is recursive,
// through the predicates of the nested preconditions: the outer assertion
// calls g, whose precondition calls k, whose precondition is not proven
// when nothing is known of y -- the outer is unproven, noting k's
// precondition (at -O2 g's own is proven by the proof shadow, k returning
// 1).  After a guard everything is proven at -O2.  A precondition calling
// its own function is not made again on the way: p's is nested once.  At
// -O0 folding alone: g's precondition is not proven either, and both are
// noted, also after the guard.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-recursive.C and
// p4405-nested-recursive-O0.C (gnu_gcc, PC1-GCC).

int k(int x) pre(x > 0);        // o2-note {{precondition of a call in the predicate cannot be proven}} \
                                // o0-note 2 {{precondition of a call in the predicate cannot be proven}}
int k(int) { return 1; }
int g(int x) pre(k(x) == 1);    // o0-note 2 {{precondition of a call in the predicate cannot be proven}}
int g(int) { return 1; }

bool p(int x) pre(x == 0 || p(x - 1)); // expected-note {{precondition of a call in the predicate cannot be proven}}

int unprovable(int y) {
  contract_assert(g(y) == 1);   // expected-error {{contract assertion cannot be proven}}
  return y;
}

int guarded(int y) {
  if (y <= 0)
    return 0;
  contract_assert(g(y) == 1);   // o0-error {{contract assertion cannot be proven}}
  return y;
}

int self(int y) {
  contract_assert(p(y));        // expected-error {{contract assertion cannot be proven}}
  return y;
}
