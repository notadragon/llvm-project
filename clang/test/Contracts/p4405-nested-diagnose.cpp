// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O2 -Rcontract-proof -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O0 -Rcontract-proof -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-configuration-file=%S/p4405-nested-diagnose.json -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0 %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169: a nested precondition analyzed
// with diagnose that is not proven gets its own warning, at its call in the
// predicate, though the predicate is never evaluated at run time (runtime
// ignore here); it does not make the outer assertion, analyzed with prove,
// unproven -- but it keeps the outer's proof conditional (E169 (10)), so
// that -fcontracts-prove-assume does not assume it.  At -O2 the proof
// shadow proves the outer ones; the nested one is proven after a guard.  At
// -O0 folding alone: each nested diagnose precondition gets its own warning
// and the outer assertions, which folding cannot decide, are not proven --
// without a note for the nested ones, which do not count for them.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-diagnose.C and
// p4405-nested-diagnose-O0.C (gnu_gcc, PC1-GCC).

namespace dd {
int g1(int x) pre(x != 0);      // expected-note {{the predicate of the contract assertion}}
int g1(int) { return 1; }
int g2(int x) pre(x != 0);      // o0-note {{the predicate of the contract assertion}}
int g2(int) { return 1; }
}

int unprovable(int y) {
  contract_assert(              // o0-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: proven, sound; with those nested in it: proven, conditional}}
    dd::g1(y) == 1);            // expected-warning {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: nested: not proven}}
  return y;
}

int guarded(int y) {
  if (y == 0)
    return 0;
  contract_assert(              // o0-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: proven, sound; with those nested in it: proven, sound; assumed}}
    dd::g2(y) == 1);            // o0-warning {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: nested: proven, sound}}
  return y;
}
