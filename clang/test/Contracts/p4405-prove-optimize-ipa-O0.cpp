// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose
// RUN: %{cc1} -O0 -emit-llvm-only -verify=on %s
// RUN: %{cc1} -O2 -emit-llvm-only -verify=on %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=0 -emit-llvm-only -verify=off %s
// RUN: %{cc1} -O0 -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ignore.o %s
// RUN: %{cc1} -O0 -emit-obj -o %t.shadow.o %s
// RUN: cmp %t.ignore.o %t.shadow.o
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=0 -Wno-contract-unproven -emit-obj -o %t.level0.o %s
// RUN: cmp %t.ignore.o %t.level0.o
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E166): in a -O0 module the interprocedural attribute inference -O0 does
// not run still runs for the proofs, at the proof level: sq is never
// inlined (noinline), but the proof module finds it does not read memory,
// so the two calls in the predicate are one and the assertion is proven,
// as at -O2.  At proof level 0 it is not.  The object code is the same as
// without the shadows.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-ipa-O0.C

[[gnu::noinline]] int sq(int x) { return x * x; }

void f(int n) {
  contract_assert(sq(n) == sq(n)); // off-warning {{contract assertion cannot be proven}}
}

// on-no-diagnostics
