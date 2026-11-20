// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,enf %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,enf %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O1 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,enf %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,obs %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,obs %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O1 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,obs %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,ign %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,ign %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -fno-contracts-prove-callee-premises -verify=expected,checked,o0 %s
// REQUIRES: x86-registered-target

// P4405, notadragon_wg21 DECISIONS.md E169: a caller-side precondition's
// analysis evaluation at a call depends on the preconditions of the calls
// in its predicate, with the call's arguments for the callee's parameters:
// `f(0)` makes g's precondition always false (folding, the argument
// substituted).  With runtime enforce or observe the predicate is also
// checked on the callee's side, where g's precondition is analyzed at its
// call in f's check (an error at `g(y)` in f, noting g's predicate); with
// runtime ignore f's predicate is only analyzed, at each call, and nothing
// is diagnosed in f.  At proof level 0 folding alone, so the guarded call is not
// proven either.
//
// The shape crashed the proof shadow at -O1 and -O2 under enforce and
// observe (llvm_llvm-project CLANG-879): -fcontracts-prove-assume's
// assumption region for f's precondition in the caller held, nested in it,
// the assumption region of the evaluation at the call of g in f's check,
// and deleting both left a deleted block to be merged.  Nothing is placed
// at the calls in an assumption's predicate any more.
//
// `checked_outer` pins E169 (7): the analysis evaluation of the
// assertion, earlier in the proof shadow, evaluates `g(y)` -- g's own
// enforce check of its precondition, inlined there, included -- which is a
// premise for the evaluation at the same call in the check, as any code
// before it is: proven there at -O1/-O2 under enforce (both diagnosed at
// -O0, and under observe, whose check is no premise).
// -fno-contracts-prove-callee-premises: with the callee premise copies
// (D4405, DECISIONS.md E199 (b)) g's observed or ignored precondition,
// inlined where the analysis evaluation evaluates `g(y)', would be a premise
// too, and prove the evaluation at the call in the check conditionally.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-nested-caller-side.C,
// p4405-nested-caller-side-O0.C and p4405-nested-caller-side-ignore.C
// (gnu_gcc, PC1-GCC); the -O1, observe and `checked_outer` cases are
// Clang's.

int g(int x) pre(x != 0);       // expected-note {{precondition of a call in the predicate is always false}} \
                                // expected-note 2 {{precondition of a call in the predicate cannot be proven}} \
                                // o0-note {{precondition of a call in the predicate cannot be proven}} \
                                // checked-note {{the predicate of the contract assertion}} \
                                // obs-note {{the predicate of the contract assertion}} \
                                // o0-note {{the predicate of the contract assertion}}
int g(int) { return 1; }

int f(int y)
  pre(g(y) == 1)                // checked-error {{contract assertion cannot be proven}} \
                                // expected-note 2 {{the predicate of the contract assertion}} \
                                // o0-note {{the predicate of the contract assertion}}
{
  return y;
}

int caller(int y) {
  return f(y);                  // expected-error {{contract assertion cannot be proven}}
}

int caller_guarded(int y) {
  if (y == 0)
    return 0;
  return f(y);                  // o0-error {{contract assertion cannot be proven}}
}

int caller_zero() {
  return f(0);                  // expected-error {{contract assertion cannot be proven}}
}

int checked_outer(int y) {
  contract_assert(              // expected-error {{contract assertion cannot be proven}}
    g(y) == 1);                 // obs-error {{contract assertion cannot be proven}} \
                                // o0-error {{contract assertion cannot be proven}}
  return y;
}
