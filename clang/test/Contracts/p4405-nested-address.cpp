// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4405, notadragon_wg21 DECISIONS.md E169, "Taking the Address of a
// Function": where the address of a function whose precondition allows no
// runtime semantic is taken, folding decides that precondition together
// with the preconditions of the calls in its predicate, with nothing known
// of the arguments.  `g(1)` is proven, so `a1` is not diagnosed; `g(0)` is
// always false and `g(x)` cannot be proven, each noted after "precondition
// here".  A call the predicate never evaluates -- the right operand of
// `true ||`, an unevaluated operand -- has nothing nested.  A nested
// precondition analyzed with diagnose (gd's, always_diagnose) is warned on
// its own, at its call, for each address taken, ahead of the error, as in
// gnu_gcc (Clang's case).  Diagnosed in Sema, so -fsyntax-only shows it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-address.C
// (gnu_gcc, PC1-GCC); `a6` is Clang's.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

constexpr int g(int x) pre(x != 0) { return 1; } // expected-note {{precondition of a call in the predicate is always false}} \
                                                 // expected-note {{precondition of a call in the predicate cannot be proven}}
int gd(int x) pre<always_diagnose>(x != 0);      // expected-note 2 {{the predicate of the contract assertion}}

void a1(int x) pre<always_prove>(g(1) == 1);
void a2(int x) pre<always_prove>(g(0) == 1);    // expected-note {{precondition here}}
void a3(int x) pre<always_prove>(g(x) == 1);    // expected-note {{precondition here}}
void a4(int x) pre<always_prove>(true || g(0) == 1);
void a5(int x) pre<always_prove>(sizeof(g(0)) == sizeof(int));
void a6(int x) pre<always_prove>(gd(x) == 1);   // expected-note 2 {{precondition here}} \
                                                // expected-warning 2 {{contract assertion cannot be proven}}

void take() {
  void (*p1)(int) = &a1;
  void (*p2)(int) = &a2;        // expected-error {{contract assertion cannot be proven for a call through the address of 'a2'}}
  void (*p3)(int) = &a3;        // expected-error {{contract assertion cannot be proven for a call through the address of 'a3'}}
  void (*p4)(int) = &a4;
  void (*p5)(int) = &a5;
  void (*p6)(int) = &a6;        // expected-error {{contract assertion cannot be proven for a call through the address of 'a6'}}
  void (*p7)(int) = &a6;        // expected-error {{contract assertion cannot be proven for a call through the address of 'a6'}}
  (void)p1; (void)p2; (void)p3; (void)p4; (void)p5; (void)p6; (void)p7;
}
