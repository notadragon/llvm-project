// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontracts-prove-assume %libcxx_flags -O2 -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontracts-prove-assume %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4405, notadragon_wg21 DECISIONS.md E196 item 1: an analysis evaluation
// that is not proven is still a premise for the later ones on its path (its
// obligation's failure does not return in the proof shadow), so `pre(x > 3)
// pre(x > 2) pre(x > 1)' gives one warning, not three.  But a proof resting
// on it is only conditional: under -fcontracts-prove-assume it never becomes
// an assumption, so the real code keeps `y > 2' (g2, g3), where a sound
// proof lets it fold (g4).  The same after an evaluation that is always false
// (g3; E195: its obligation is decided in the shadow, which finds it
// reached).  At -O0, where only folding runs, there are no premises:
// p4405-premise-unproven-O0.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-unproven.C

#include <contracts>
using namespace std::contracts::labels;

void f(int x) pre<diagnose>(x > 3) // expected-note {{predicate}}
    pre<diagnose>(x > 2) pre<diagnose>(x > 1);

void g1(int y) {
  // Exactly one warning at the call.
  f(y); // expected-warning {{cannot be proven}} \
        // expected-remark {{proof shadow: not proven}} \
        // expected-remark 2 {{proof shadow: proven, conditional}}
}

int g2(int y) {
  contract_assert<diagnose>(y > 3); // expected-warning {{cannot be proven}} \
                                    // expected-remark {{not proven}}
  contract_assert<diagnose>(y > 2); // expected-remark {{proven, conditional}}
  contract_assert<diagnose>(y > 1); // expected-remark {{proven, conditional}}
  return y > 2;
}

int g3(int y) {
  const int k = 1;
  contract_assert<diagnose>(k == 0); // expected-warning {{always false}} \
                                     // expected-remark {{always false}}
  contract_assert<diagnose>(y > 2);  // expected-remark {{proven, conditional}}
  return y > 2;
}

int g4(int y) {
  if (y <= 2)
    __builtin_unreachable();
  contract_assert<diagnose>(y > 2); // expected-remark {{sound; assumed}}
  return y > 2;
}

// The real code: only g4's comparison folds.
// CHECK-LABEL: define {{.*}} @_Z2g2i(
// CHECK: icmp sgt i32 %y, 2
// CHECK-LABEL: define {{.*}} @_Z2g3i(
// CHECK: icmp sgt i32 %y, 2
// CHECK-LABEL: define {{.*}} @_Z2g4i(
// CHECK-NOT: icmp
// CHECK: ret i32 1
