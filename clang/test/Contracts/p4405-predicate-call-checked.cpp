// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-configuration-file=%S/p4405-predicate-call-checked.json -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify=expected,o0 %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-predicate-call-checked.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify=expected,o2 %s
// REQUIRES: x86-registered-target

// P4405: a call in a contract predicate that a check evaluates is
// evaluated, so its callee's caller-side prove and diagnose evaluations
// are diagnosed, at the call, where the check is emitted: here g's
// callee-side precondition and assertion are enforce, so g's body
// evaluates check(v) twice, and check's caller-side precondition is prove.
// g's own caller-side precondition is diagnosed at the call of g, noting
// check's, on which it depends (notadragon_wg21 DECISIONS.md E169).  h's
// checks evaluate check(v) only after `v > 5`, which folding cannot use:
// at -O2 the proof shadow proves check's precondition there.  At -O2 the
// evaluation in g's assertion is proven (conditionally) by g's
// precondition check before it, whose call of check -- check's own enforce
// check of its precondition included -- is a premise for what follows, as
// for any two calls of check in turn.  Where the predicate is never
// evaluated, check's precondition is not reported on its own
// (p4405-shadow-predicate-call.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-predicate-call-checked.C
// and p4405-predicate-call-checked-O2.C (gnu_gcc, P5-GCC and PC1-GCC).

bool check(int v)
  pre(v > 0);                   // expected-note {{the predicate of the contract assertion}} \
                                // o0-note 3 {{the predicate of the contract assertion}} \
                                // expected-note {{precondition of a call in the predicate cannot be proven}}

void g(int v)
  pre(check(v))                 // expected-note {{the predicate of the contract assertion}} \
                                // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(check(v));    // o0-error {{contract assertion cannot be proven}}
}

void use(int v) {
  g(v);                         // expected-error {{contract assertion cannot be proven}}
}

void h(int v)
  pre(v > 5 && check(v))        // o0-error {{contract assertion cannot be proven}}
{
  contract_assert(v > 5 && check(v)); // o0-error {{contract assertion cannot be proven}}
}
