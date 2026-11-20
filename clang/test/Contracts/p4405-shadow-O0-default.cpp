// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0
// RUN: %{cc1} -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ignore.o %s
// RUN: %{cc1} -fcontract-evaluation-semantic=prove -emit-obj -o %t.prove.o %s
// RUN: cmp %t.ignore.o %t.prove.o
// RUN: %{cc1} -fcontract-evaluation-semantic=prove -fno-contracts-prove-assume -emit-obj -o %t.noassume.o %s
// RUN: cmp %t.ignore.o %t.noassume.o
// RUN: %{cc1} -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -fcontracts-prove-optimize=0 -emit-obj -o %t.level0.o %s
// RUN: cmp %t.ignore.o %t.level0.o
// RUN: %{cc1} -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -fno-contracts-prove-shadow -emit-obj -o %t.noshadow.o %s
// RUN: cmp %t.ignore.o %t.noshadow.o
// REQUIRES: x86-registered-target

// P4405 stage 3, D4405's proof optimization level (notadragon_wg21
// DECISIONS.md E162, E166): at -O0 the default proof level is 2, and the
// proof shadows are optimized at that level although the code is not, so
// p4405-shadow-O0.cpp's evaluations are proven as at -O2.  The code is the
// same as without the shadows and with ignore; -fcontracts-prove-assume, on
// by default, keeps the assumptions of the sound proofs, which the end of
// the -O0 pipeline deletes with their predicates' code, leaving the code as
// it was (DECISIONS.md E167).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-O0-default.C

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);
  contract_assert(2 > 1);
  int c = n < 0 ? 0 : 1;
  contract_assert(c >= 0);
  return c;
}

// expected-no-diagnostics
