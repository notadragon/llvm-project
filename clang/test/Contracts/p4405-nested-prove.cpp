// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -Rcontract-proof -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -Rcontract-proof -emit-llvm-only -verify=expected,o2 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify=expected,o0 %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169: an analysis evaluation depends
// on the analysis evaluations of the preconditions of the calls in its
// predicate, though the predicate is never evaluated at run time (runtime
// ignore here).  A nested prove precondition that cannot be proven, or is
// always false, makes the outer assertion unproven: "contract assertion
// cannot be proven" at the outer, with a note at the nested precondition,
// which is not reported on its own.  At -O2 the proof shadow decides both,
// the nested one where its call is evaluated in the outer's region: proven
// after a guard, and caught before the undefined behavior in the callee
// (`10 / 0`) could make the rest of the path unreachable, whether folding
// decides the nested one (`g3(0)`, with the argument substituted) or the
// shadow does (`g5(y)` after `y != 0` returned).  The -Rcontract-proof
// remarks show each obligation's own result and, for an outer one, its
// result with those nested in it (gnu_gcc's dump scans).  At proof level 0 folding
// alone decides each evaluation: every outer assertion is unproven, noting
// the nested precondition unless folding proves it (`g4(5)`).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-prove.C and
// p4405-nested-prove-O0.C (gnu_gcc, PC1-GCC).

int g1(int x) pre(x != 0);      // expected-note {{precondition of a call in the predicate cannot be proven}}
int g1(int) { return 1; }
int g2(int x) pre(x != 0);      // o0-note {{precondition of a call in the predicate cannot be proven}}
int g2(int) { return 1; }
int g3(int x) pre(x != 0);      // expected-note {{precondition of a call in the predicate is always false}}
int g3(int x) { return 10 / x; }
int g4(int x) pre(x != 0);
int g4(int) { return 1; }
int g5(int x) pre(x != 0);      // expected-note {{precondition of a call in the predicate cannot be proven}}
int g5(int x) { return 10 / x; }

int unprovable(int y) {
  contract_assert(g1(y) == 1);  // expected-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: proven, sound; with those nested in it: not proven}} \
                                // o2-remark {{proof shadow: nested: not proven}}
  return y;
}

int guarded(int y) {
  if (y == 0)
    return 0;
  contract_assert(g2(y) == 1);  // o0-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: proven, sound; with those nested in it: proven, sound; assumed}} \
                                // o2-remark {{proof shadow: nested: proven, sound}}
  return y;
}

int always_false(int y) {
  contract_assert(g3(0) != 7);  // expected-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: not proven; with those nested in it: not proven}}
  return y;
}

int guarded_zero(int y) {
  if (y != 0)
    return 0;
  contract_assert(g5(y) != 7);  // expected-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: not proven; with those nested in it: not proven}} \
                                // o2-remark {{proof shadow: nested: not proven}}
  return y;
}

int provable(int y) {
  contract_assert(g4(5) == 1);  // o0-error {{contract assertion cannot be proven}} \
                                // o2-remark {{proof shadow: proven, sound; assumed}}
  return y;
}
