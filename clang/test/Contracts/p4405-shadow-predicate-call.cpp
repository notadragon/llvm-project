// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3, notadragon_wg21 DECISIONS.md E169: a call in the predicate
// of a prove or diagnose evaluation is not evaluated by the program, but
// the evaluation depends on the analysis evaluations of its callee's
// preconditions all the same: here the precondition of g's call to check
// is caller-side prove, check's own caller-side precondition is prove and
// cannot be proven, so g's precondition is not proven, noting check's,
// which is not reported on its own -- at -O0, where no proof shadow is
// made, as at -O2, where the proof shadow (and, under
// -fcontracts-prove-assume, the assumption a sound proof may become) holds
// the predicate.  g's own callee-side evaluation is ignore.  A call in a
// predicate a check evaluates is also analyzed at the call
// (p4405-predicate-call-checked.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-predicate-call.C,
// p4405-shadow-predicate-call-O0.C and
// p4405-shadow-predicate-call-no-assume.C (gnu_gcc).

bool check(int v)
  pre(v > 0);                   // expected-note {{precondition of a call in the predicate cannot be proven}}

void g(int v)
  pre(check(v))                 // expected-note {{the predicate of the contract assertion}}
{}

void use(int v) {
  g(v);                         // expected-error {{contract assertion cannot be proven}}
}
