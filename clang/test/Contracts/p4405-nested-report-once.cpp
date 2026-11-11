// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-nested-report-once.json -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-nested-report-once.json -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-configuration-file=%S/p4405-nested-report-once.json -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0 %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169 (9) and (11): a nested diagnose
// precondition in a predicate that is also checked at run time is warned
// once at its call, not twice (the analysis evaluation's nested evaluation
// and the evaluation at the same call in the check give the same warning,
// E120's local, stateless suppression).  Only the analysis and the check
// of the same assertion in the same place are merged: the same nested
// evaluation under several outer evaluations -- dd::g5's precondition in
// f's precondition, analyzed at each call of f -- is warned once per
// outer.  At -O2 the outer assertions are proven, conditionally
// (E169 (10)); at proof level 0 folding cannot prove them.
//
// `pinned': f2's precondition is checked in f2 (callee side) and analyzed
// at each call of f2; the check's evaluation at dd::g5's call and the
// nested evaluation of the analysis at the caller are not the same
// assertion in the same place, so both warn (E169 (15)).
//
// Before PC5-Clang the warnings of the several outers were merged into one,
// and so were the two of `pinned'.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-report-once.C
// and p4405-nested-report-once-O0.C (gnu_gcc, PC4-GCC).

namespace dd {
int g5(int x) pre(x != 0);      // expected-note 9 {{the predicate of the contract assertion}}
int g5(int) { return 1; }
}

// Checked and analyzed in the same place: once.
int single(int y) {
  contract_assert(              // o0-error {{contract assertion cannot be proven}}
    dd::g5(y) == 1);            // expected-warning {{contract assertion cannot be proven}}
  return y;
}

// Two assertions, each its own pair: once each.
int two(int y) {
  contract_assert(              // o0-error {{contract assertion cannot be proven}}
    dd::g5(y) == 1);            // expected-warning {{contract assertion cannot be proven}}
  contract_assert(              // o0-error {{contract assertion cannot be proven}}
    dd::g5(y + 1) == 1);        // expected-warning {{contract assertion cannot be proven}}
  return y;
}

// A postcondition, analyzed and checked in p: once.
int p(int y)
  post(r:                       // o0-error {{contract assertion cannot be proven}}
       dd::g5(r) == 1)          // expected-warning {{contract assertion cannot be proven}}
{
  return y;
}

// f's precondition is only analyzed (preconditions are ignore at run time
// here), at each of three calls: three outers, three warnings.
int f(int y)
  pre(dd::g5(y) == 1);          // expected-warning 3 {{contract assertion cannot be proven}} \
                                // o0-note 3 {{the predicate of the contract assertion}}
int f(int y) { return y; }

int c1(int y) {
  return f(y);                  // o0-error {{contract assertion cannot be proven}}
}

int c2(int y) {
  return f(y) + f(y + 1);       // o0-error 2 {{contract assertion cannot be proven}}
}

namespace pinned {
int f2(int y)
  pre(dd::g5(y) == 1);          // expected-warning 2 {{contract assertion cannot be proven}} \
                                // o0-note {{the predicate of the contract assertion}}
int f2(int y) { return y; }
}

int c3(int y) {
  return pinned::f2(y);         // o0-error {{contract assertion cannot be proven}}
}
