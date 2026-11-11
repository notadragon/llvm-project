// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-nested-checked.json -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-nested-checked.json -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-configuration-file=%S/p4405-nested-checked.json -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,o0 %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169 (5): when a predicate is also
// evaluated at run time (the assertions' runtime semantic is enforce here),
// its analysis evaluation depends on the nested preconditions as when it is
// only analyzed, and each nested precondition is analyzed at its call in
// the check as well, as at any call: two evaluations, two diagnostics.  At
// -O2 the proof shadow decides the evaluation at the call in the check
// too.  The preconditions' own runtime semantic is ignore: an enforce check
// in g1, inlined into the shadow where the analysis evaluation evaluates
// `g1(y)`, would be a premise for the evaluation at the call in the check,
// as any code before it is, and prove it conditionally (E169 (7),
// p4405-nested-caller-side.cpp).  At proof level 0 folding alone: every outer
// assertion is unproven, noting its nested precondition unless that is
// proven, and every nested precondition is reported at its call in the
// check unless it is proven.
//
// Before PC2-Clang (llvm_llvm-project CLANG-880), at -O2 the evaluations at
// the calls in the checks were lost: -fcontracts-prove-assume's assumption
// in the real function emits the predicate first, and the evaluation met
// at a call there, dropped as one in a predicate, was taken for the one at
// the same call in the check (`unguarded`, `always_false`).
//
// A nested diagnose precondition in a checked predicate (`report_once`) is
// warned once at its call, though the nested evaluation and the one at the
// call in the check both make the warning (E169 (9), E120: local,
// stateless suppression).  At -O2 the outer is proven, conditionally.
// Several outer evaluations each warn (E169 (11),
// p4405-nested-report-once.cpp).
// -fno-contracts-prove-callee-premises: with the callee premise copies
// (D4405, DECISIONS.md E199 (b)) g1's ignored precondition, inlined there,
// would be a premise too, and prove the evaluation at the call in the check
// conditionally.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-checked.C and
// p4405-nested-checked-O0.C (gnu_gcc, PC1-GCC; `report_once` mirrored there
// in PC4-GCC).

int g1(int x) pre(x != 0);      // expected-note {{precondition of a call in the predicate cannot be proven}} \
                                // expected-note {{the predicate of the contract assertion}}
int g1(int) { return 1; }
int g2(int x) pre(x != 0);      // o0-note {{precondition of a call in the predicate cannot be proven}} \
                                // o0-note {{the predicate of the contract assertion}}
int g2(int) { return 1; }
int g3(int x) pre(x != 0);      // expected-note {{precondition of a call in the predicate is always false}} \
                                // o0-note {{the predicate of the contract assertion}}
int g3(int x) { return 10 / x; }
int g4(int x) pre(x != 0);      // o0-note {{precondition of a call in the predicate cannot be proven}} \
                                // o0-note {{the predicate of the contract assertion}}
int g4(int x) { return 10 / x; }

namespace dd {
int g5(int x) pre(x != 0);      // expected-note {{the predicate of the contract assertion}}
int g5(int) { return 1; }
}

int unguarded(int y) {
  contract_assert(              // expected-error {{contract assertion cannot be proven}}
    g1(y) == 1);                // expected-error {{contract assertion cannot be proven}}
  return y;
}

int guarded(int y) {
  if (y == 0)
    return 0;
  contract_assert(              // o0-error {{contract assertion cannot be proven}}
    g2(y) == 1);                // o0-error {{contract assertion cannot be proven}}
  return y;
}

// At -O2 the evaluation at the call in the check is not diagnosed: folding
// finds it always false, but the proof shadow decides whether it is reached
// (DECISIONS.md E195), and the analysis evaluation's copy of g3(0), earlier
// in the shadow, divides by zero, so it is not (E169 (7): a conditional
// proof).  At proof level 0 both are diagnosed.
int always_false(int y) {
  contract_assert(              // expected-error {{contract assertion cannot be proven}}
    g3(0) != 7);                // o0-error {{contract predicate is always false}}
  return y;
}

// The shape gnu_gcc's GCC-953 was found with: the outer predicate is not
// provable, but the nested precondition is, after the guard.
int guarded_outer_unprovable(int y) {
  if (y == 0)
    return 0;
  contract_assert(              // expected-error {{contract assertion cannot be proven}}
    g4(y) != 7);                // o0-error {{contract assertion cannot be proven}}
  return y;
}

int report_once(int y) {
  contract_assert(              // o0-error {{contract assertion cannot be proven}}
    dd::g5(y) == 1);            // expected-warning {{contract assertion cannot be proven}}
  return y;
}
