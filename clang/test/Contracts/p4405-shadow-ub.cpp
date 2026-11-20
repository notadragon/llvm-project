// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: in the proof shadow a path with undefined behavior is
// unreachable, exactly as an assumed predicate is, and relying on it is
// sound: language undefined behavior is a guaranteed premise.  After *p,
// p is not null; x + 1 does not overflow.  Proving that the undefined
// behavior does not happen is the business of an earlier assertion
// (P3100's implicit ones), not of the later one.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-ub.C

int deref(int *p) {
  int v = *p;
  contract_assert(p != nullptr);  // expected-remark {{proof shadow: proven, sound}}
  return v;
}

int incremented(int x) {
  int y = x + 1;
  contract_assert(y > x);         // expected-remark {{proof shadow: proven, sound}}
  return y;
}

int unknown(int *p) {
  contract_assert(p != nullptr);  // expected-remark {{proof shadow: not proven}} \
                                  // expected-error {{contract assertion cannot be proven}}
  return *p;
}
