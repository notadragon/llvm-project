// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only -Rcontract-proof
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=1 -verify=assume %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -verify=zero %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-assume -verify=zero %s
// RUN: %{cc1} -O0 -verify=assume %s
// RUN: %{cc1} -O0 -fno-contracts-prove-assume -verify=noassume %s
// RUN: %{cc1} -O3 -fcontracts-prove-optimize=3 -fno-contracts-prove-assume -verify=noassume %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=follow -verify=assume %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -verify=noshadow %s
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E205): -fcontracts-prove-assume is on from level 1, with the proof shadow,
// and one given explicitly, in either form, wins.  k's evaluation is proven
// soundly in its proof shadow, and becomes an assumption where
// -fcontracts-prove-assume is on, at -O0 too (DECISIONS.md E167).  At level
// 0 there is no proof shadow; one given explicitly runs at level 0
// (DECISIONS.md E208 (a)) and does not prove k.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-0.C,
// -0-shadow.C, -0-explicit.C, -1.C, -3-explicit-off.C (their k)

int k(int i, int n) {
  if (i < n)
    contract_assert(i < n);    // assume-remark {{proof shadow: proven, sound; assumed}} noassume-remark-re {{proof shadow: proven, sound{{$}}}} noshadow-error {{contract assertion cannot be proven}} zero-error {{contract assertion cannot be proven}} zero-remark {{proof shadow: not proven}}
  return i;
}
