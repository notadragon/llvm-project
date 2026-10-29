// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-predicate-call-checked.json -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s

// P4405: a call in a contract predicate that a check evaluates is
// evaluated, so its callee's caller-side prove and diagnose evaluations
// are diagnosed, at the call, where the check is emitted: here g's
// callee-side precondition and assertion are enforce, so g's body
// evaluates check(v) twice, and check's caller-side precondition is prove.
// g's own caller-side precondition is diagnosed at the call of g.  Where
// the predicate is never evaluated nothing is diagnosed
// (p4405-shadow-predicate-call.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-predicate-call-checked.C
// (gnu_gcc, P5-GCC).

bool check(int v)
  pre(v > 0);                   // expected-note 2 {{the predicate of the contract assertion}}

void g(int v)
  pre(check(v))                 // expected-note {{the predicate of the contract assertion}} \
                                // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(check(v));    // expected-error {{contract assertion cannot be proven}}
}

void use(int v) {
  g(v);                         // expected-error {{contract assertion cannot be proven}}
}
