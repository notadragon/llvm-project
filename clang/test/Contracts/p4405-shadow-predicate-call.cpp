// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a call in the predicate of a prove or diagnose evaluation
// is not evaluated by the program, so its own caller-side evaluations are
// not diagnosed -- at -O0, where the predicate is never emitted, nor at
// -O2, where the proof shadow (and, under -fcontracts-prove-assume, the
// assumption a sound proof may become) emits it.  Here the precondition
// of g's call to check is caller-side prove, and check's own caller-side
// precondition is never diagnosed.  (Clang's own test.)

bool check(int v)
  pre(v > 0);

void g(int v)
  pre(check(v))                 // expected-note {{the predicate of the contract assertion}}
{}

void use(int v) {
  g(v);                         // expected-error {{contract assertion cannot be proven}}
}
