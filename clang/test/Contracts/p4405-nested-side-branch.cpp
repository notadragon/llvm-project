// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-nested-diagnose.json -fcontract-analysis-semantic=prove -fcontracts-prove-assume -triple x86_64-unknown-linux-gnu -O2 -Rcontract-proof -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-nested-diagnose.json -fcontract-analysis-semantic=prove -fcontracts-prove-assume -triple x86_64-unknown-linux-gnu -O0 -Rcontract-proof -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169 (12): a nested obligation's
// region is a side branch of the path that never joins it again, so its
// failure is no premise for the other proofs in the same shadow: `y >= 0'
// after the guard is proven soundly, and assumed under
// -fcontracts-prove-assume, next to g1's failed nested precondition, whose
// outer assertion is unproven.  E169 (10): a nested diagnose precondition
// not proven (dd::g5's) makes its own outer conditional, so not assumed,
// while the outer's own result and the proof after it stay sound.  The
// -Rcontract-proof remarks stand for gnu_gcc's dump scans.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-side-branch.C
// (gnu_gcc, PC4-GCC).

int g1(int x) pre(x != 0);      // expected-note {{precondition of a call in the predicate cannot be proven}}
int g1(int) { return 1; }
namespace dd {
int g5(int x) pre(x != 0);      // expected-note {{the predicate of the contract assertion}}
int g5(int) { return 1; }
}
int sink(int);

int side(int y) {
  contract_assert(g1(y) == 1);  // expected-error {{contract assertion cannot be proven}} \
                                // expected-remark {{proof shadow: proven, sound; with those nested in it: not proven}} \
                                // expected-remark {{proof shadow: nested: not proven}}
  if (y < 0)
    return 0;
  contract_assert(y >= 0);      // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(y);
}

int side_diagnose(int y) {
  if (y < 0)
    return 0;
  contract_assert(dd::g5(y) == 1);  // expected-warning {{contract assertion cannot be proven}} \
                                    // expected-remark {{proof shadow: proven, sound; with those nested in it: proven, conditional}} \
                                    // expected-remark {{proof shadow: nested: not proven}}
  contract_assert(y >= 0);      // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink(y);
}
