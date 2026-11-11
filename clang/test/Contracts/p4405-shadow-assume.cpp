// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-shadow-assume.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3 with the P3100 assume semantic: an assume precondition or
// postcondition reaches its own function's proof shadow as a guaranteed
// assumption, so a proof relying on it is sound -- as an assume
// contract_assert's always did (p4405-shadow-previous.cpp).  The
// configuration selects assume by line; every other evaluation proves.
//
// Clang lowers the assume semantic like ignore in the real code (CLANG-111,
// deferred), so where a function with an assume postcondition is inlined
// into a caller's shadow nothing is assumed of its result: use_post's
// assertion is not proven (GCC, which lowers assume as an assumption,
// proves it).  -fcontracts-prove-assume does not change that: the
// postcondition is assume, not a proven prove or diagnose.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-assume.C

int sink(int);

int pre_assumed(int x)
  pre(x > 0)                    // line 22: assume
{
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound}}
  return sink(x);
}

static int post_assumed(int x)
  post(r: r > 0)                // line 29: assume
{
  return sink(x);
}

int use_post(int x) {
  int r = post_assumed(x);
  contract_assert(r > 0);       // expected-remark {{proof shadow: not proven}} \
                                // expected-error {{contract assertion cannot be proven}}
  return r;
}
