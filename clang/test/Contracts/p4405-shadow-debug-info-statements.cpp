// DEFINE: %{O} = -O0
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -debug-info-kind=constructor -dwarf-version=5 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %{O} -emit-obj
// DEFINE: %{check} = %{cc1} -fcontracts-prove-optimize=0 -o %t.ref.o %s && \
// DEFINE:   %{cc1} -o %t.shadow.o %s && cmp %t.ref.o %t.shadow.o && \
// DEFINE:   %{cc1} -fno-contracts-prove-shadow -o %t.noshadow.o %s && cmp %t.ref.o %t.noshadow.o && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0 -o %t.ignore.o %s && cmp %t.ref.o %t.ignore.o
// RUN: %{check}
// REDEFINE: %{O} = -Og
// RUN: %{check}
// REDEFINE: %{O} = -O1
// RUN: %{check}
// REDEFINE: %{O} = -O2
// RUN: %{check}
// REDEFINE: %{O} = -O3
// RUN: %{check}
// REQUIRES: x86-registered-target

// D4405 proof shadows and -g: the statement shapes of
// p4405-prove-optimize-same-code-O0.cpp -- an obligation all a statement
// holds: the arm of an `if', of a short-circuit condition, of a nested
// `if', of an `if' with an `else', a `switch' case, the body of a `for' or
// `while' loop; a precondition and a postcondition; a predicate calling a
// function the real code does not call; a predicate loading a `bool' --
// leave the line table and the rest of the debug information as they are
// without the shadows (which are emitted without debug information).  The
// object code, the debug sections included, is the same with the shadows,
// without them, at proof level 0 and with every evaluation ignore, at -O0,
// -Og, -O1 (proofs at level 2), -O2 and -O3.  f5, f8, f9 and f13 are the
// statement shapes whose markers gnu_gcc loses (GCC-998).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-debug-info-statements.C,
// open-bug-p4405-shadow-debug-info-residue.C (f5, f8, f9, f13)

struct S { int x; volatile int v; };
int g(int);

int f1(int i, int n) { if (i < n) contract_assert(n > 0); return 1; }
int f2(S *p, int n) { if (p->x < n) contract_assert(n > 0); return 2; }
int f3(int i, int n) { if (g(i) < n) contract_assert(n > 0); return 3; }
int f4(S *p, int n) { if (p->v < n) contract_assert(n > 0); return 4; }
int f5(int i, int n) {
  if (i < n)
    contract_assert(n > 0);
  else
    contract_assert(n < 0);
  return 5;
}
int f6(S *p, int i, int n) { if (i < n && p->x > 0) contract_assert(n > 0); return 6; }
int f7(int i, int n) { if (i < n) { if (n > 3) contract_assert(i > 0); } return 7; }
int f8(int i, int n) {
  switch (i) {
  case 1:
    contract_assert(n > 0);
    break;
  default:
    break;
  }
  return 8;
}
int f9(const int *p, int n) { for (int j = 0; j < n; ++j) contract_assert(p[j] > 0); return 9; }
int f12(int i, int n) pre(i < n) post(r : r > 0) { if (i > 0) contract_assert(n > 1); return 12; }
int f13(int i, int n) { if (i < n) contract_assert(g(n) > 0); return 13; }
int f14(int i, int n) { while (i < n) { contract_assert(i < n); ++i; } return i; }
bool f15(bool b, int *p) { contract_assert(!b || *p > 0); return b; }
