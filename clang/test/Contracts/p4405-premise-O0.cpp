// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0 -fcontract-configuration-file=%S/p4405-premise-O0.json -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang) at -O0: there is no shadow, so
// folding is final and nothing a callee's assertion implies is used, whatever
// its linkage or inlining; nothing is assumed, and no call is made
// always_inline.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-O0.C (gnu_gcc,
// whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

[[gnu::always_inline]] static inline void h_static_always (int x) pre (x > 0) { sink (x); }

int d_static_always (int x)
{
  h_static_always (x);
  contract_assert (x > 0);  // expected-warning {{contract assertion cannot be proven}}
  return 0;
}

static void h_static_auto (int x) pre (x > 0) { sink (x); }

int d_static_auto (int x)
{
  h_static_auto (x);
  contract_assert (x > 0);  // expected-warning {{contract assertion cannot be proven}}
  return 0;
}

[[gnu::always_inline]] inline void h_inline_always (int x) pre (x > 0) { sink (x); }

int d_inline_always (int x)
{
  h_inline_always (x);
  contract_assert (x > 0);  // expected-warning {{contract assertion cannot be proven}}
  return 0;
}

inline void h_inline_auto (int x) pre (x > 0) { sink (x); }

int d_inline_auto (int x)
{
  h_inline_auto (x);
  contract_assert (x > 0);  // expected-warning {{contract assertion cannot be proven}}
  return 0;
}
