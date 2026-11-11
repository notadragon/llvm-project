// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O3 -funroll-loops -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: an evaluation the proof shadow duplicates -- by unrolling
// a loop, or in each variant of a constructor -- is diagnosed once, as is
// a postcondition of a function with several returns.  -verify fails on a
// repeated error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-unroll.C

int sum(const int *a) {
  int s = 0;
  for (int i = 0; i < 4; ++i) {
    contract_assert(a[i] > 0);  // expected-error {{contract assertion cannot be proven}}
    s += a[i];
  }
  return s;
}

void *operator new(decltype(sizeof 0), void *p) noexcept;

struct V { int v; V(int x) : v(x) {} };

// Two constructor bodies (complete and base), one evaluation.
struct D : virtual V {
  D(int x) : V(x) {
    contract_assert(x != 3);    // expected-error {{contract assertion cannot be proven}}
  }
};

D make(int x) { return D(x); }
D *make_base(void *p, int x) {
  struct E : D { E(int x) : V(x), D(x) {} };
  return new (p) E(x);
}

int returns(int x)
  post(r: r > 0)
  post(r: r > 5)                // expected-error {{contract assertion cannot be proven}}
{
  if (x > 0)
    return x;
  if (x < -5)
    return -x;
  return 1;
}
