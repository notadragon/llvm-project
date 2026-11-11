// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-shadow-previous.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: the proof shadow assumes earlier evaluations.  A proof
// relying on one with a terminating semantic (enforce, quick_enforce) or
// assume is sound; one relying on ignore or observe is conditional.  Both
// suppress the diagnostic, so the -Rcontract-proof remark tells them
// apart.  An unrelated earlier observe still makes every proof in that
// shadow conditional (an assumption can affect what precedes it).  The
// configuration selects each earlier evaluation's semantic by its line;
// every other evaluation proves.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-previous.C
// (GCC shows the results in its -fdump-tree-contract_shadow dump)

int sink(int);

int enforced(int x) {
  contract_assert(x > 0);       // line 19: enforce
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound}}
  return sink(x);
}

int quick(int x) {
  contract_assert(x > 0);       // line 25: quick_enforce
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound}}
  return sink(x);
}

int assumed(int x) {
  contract_assert(x > 0);       // line 31: assume
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, sound}}
  return sink(x);
}

int observed(int x) {
  contract_assert(x > 0);       // line 37: observe
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, conditional}}
  return sink(x);
}

int ignored(int x) {
  contract_assert(x > 0);       // line 43: ignore
  contract_assert(x > 0);       // expected-remark {{proof shadow: proven, conditional}}
  return sink(x);
}

int unrelated(int x, int y) {
  if (x > 0) {
    contract_assert(x > 0);     // expected-remark {{proof shadow: proven, conditional}}
    sink(x);
  }
  contract_assert(y > 0);       // line 53: observe
  return 0;
}
