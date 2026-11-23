// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: each template instantiation has its own proof shadow, so
// an evaluation is proven or diagnosed per instantiation.  An inline
// function that is used is proven in its shadow; one that is never used is
// never emitted, so it is not diagnosed at all (Clang's stage 1 behaviour;
// GCC diagnoses it, DECISIONS.md E130), and an uninstantiated template not
// at all.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template.C
// (except the unused inline function, E130)

template <int N>
int f(int x) {
  if (x < 10)
    contract_assert(x + N < 15);  // expected-error {{contract assertion cannot be proven}}
  return x;
}

template int f<5>(int);  // proven: x + 5 < 15
template int f<6>(int);  // diagnosed: x = 9 gives 15; expected-note {{in instantiation of function template specialization 'f<6>' requested here}}

template <int N>
int never(int x) {
  contract_assert(x > N);
  return x;
}

inline int used(int x) {
  if (x > 0)
    contract_assert(x != 0);
  return x;
}

inline int unused(int x) {
  if (x > 0)
    contract_assert(x != 0);
  return x;
}

int call_used(int x) { return used(x); }
