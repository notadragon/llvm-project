// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=ignore %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4405, notadragon_wg21 DECISIONS.md E196 item 1, at proof level 0: only folding
// runs, which takes no premise from an earlier evaluation, so each of the
// three preconditions is warned at the call (at -O1 and above the proof
// shadow gives one warning: p4405-premise-unproven.cpp).  What -O0 catches
// is not the goal (E195's answer; E200).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-unproven-O0.C

#include <contracts>
using namespace std::contracts::labels;

void f(int x) pre<diagnose>(x > 3)  // expected-note {{predicate}}
    pre<diagnose>(x > 2)            // expected-note {{predicate}}
    pre<diagnose>(x > 1);           // expected-note {{predicate}}

void g1(int y) {
  f(y); // expected-warning 3 {{cannot be proven}}
}
