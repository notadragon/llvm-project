// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -emit-llvm-only -round-trip-args
// RUN: %{cc1} -O0 -verify=on %s
// RUN: %{cc1} -O2 -verify=on %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=0 -verify=on,off %s
// RUN: %{cc1} -O0 -relaxed-aliasing -verify=on,alias %s
// RUN: %{cc1} -O2 -relaxed-aliasing -verify=on,alias %s
// RUN: %{cc1} -O0 -fno-inline -verify=on,noinline %s
// RUN: %{cc1} -O2 -fno-inline -verify=on,noinline %s
// RUN: %{cc1} -O0 -fno-inline-functions -verify=on,noinline %s
// RUN: %{cc1} -O0 -finline-hint-functions -verify=on %s
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168):
// an -O0 module proving at the default level, 2, gives its proofs what the
// level gives an -O2 module and -O0 does not -- TBAA (h: storing a float
// cannot change an int), the range of a bool load (r), and inlining (k: an
// inline callee, inlined although -O0 marks every function noinline and
// optnone) -- and the flags the user gave still apply (E165):
// -fno-strict-aliasing (-relaxed-aliasing) loses h, -fno-inline and -fno-inline-functions lose
// k (-finline-hint-functions keeps it: clamp2 is declared inline), as at
// -O2.  A function's own optnone is kept: its body is never a proof's.
//
// Mirror: none (GCC's -O0 counterparts: gcc/testsuite/g++.dg/contracts/cpp26/
// p4405-prove-optimize-ipa-O0.C, p4405-shadow-O2-attribute.C)

int h(int *ip, float *fp) {
  *ip = 1;
  *fp = 1.0f;
  contract_assert(*ip == 1); // off-warning {{contract assertion cannot be proven}} alias-warning {{contract assertion cannot be proven}}
  return 0;
}

int r(bool *b) {
  bool x = *b;
  contract_assert(x == 0 || x == 1); // off-warning {{contract assertion cannot be proven}}
  return x;
}

inline int clamp2(int v) { return v < 0 ? 0 : v; }
int k(int v) {
  int c = clamp2(v);
  contract_assert(c >= 0); // off-warning {{contract assertion cannot be proven}} noinline-warning {{contract assertion cannot be proven}}
  return c;
}

[[clang::optnone]] inline int clamp3(int v) { return v < 0 ? 0 : v; }
int m(int v) {
  int c = clamp3(v);
  contract_assert(c >= 0); // on-warning {{contract assertion cannot be proven}}
  return c;
}
