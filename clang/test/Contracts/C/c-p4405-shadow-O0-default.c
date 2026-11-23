// DEFINE: %{cc1} = %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0
// RUN: %{cc1} -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ignore.o %s
// RUN: %{cc1} -fcontract-evaluation-semantic=prove -emit-obj -o %t.prove.o %s
// RUN: cmp %t.ignore.o %t.prove.o
// REQUIRES: x86-registered-target

// P4405 stage 3 in C, D4405's proof optimization level (notadragon_wg21
// DECISIONS.md E162, E166): at -O0 the default proof level is 2, so
// c-p4405-shadow-O0.c's evaluations are proven as at -O2, and the code is
// the same as with ignore.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-shadow-O0-default.c

int f(int i, int n) {
  if (i < n)
    _ContractAssert(i < n);
  _ContractAssert(2 > 1);
  int c = n < 0 ? 0 : 1;
  _ContractAssert(c >= 0);
  return c;
}

// expected-no-diagnostics
