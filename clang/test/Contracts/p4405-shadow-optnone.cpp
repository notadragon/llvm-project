// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a function not optimized, by its own optnone attribute or
// '#pragma clang optimize off', gets no proof shadow in a -O2 compilation,
// so folding is final for it.  Clang's counterpart of GCC's optimize
// attribute.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-O0-attribute.C
// (GCC's optimize("O0") attribute; Clang's own test)

[[clang::optnone, gnu::noinline]] int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);     // expected-error {{contract assertion cannot be proven}}
  return i;
}

#pragma clang optimize off
int h(int i, int n) {
  if (i < n)
    contract_assert(i < n);     // expected-error {{contract assertion cannot be proven}}
  return i;
}
#pragma clang optimize on

int g(int i, int n) {
  if (i < n)
    contract_assert(i < n);
  return i;
}
