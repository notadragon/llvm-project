// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0 -Wno-contract-unproven -discard-value-names -emit-llvm
// DEFINE: %{diag} = -fcontract-evaluation-semantic=diagnose
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0 -o %t.ref.ll %s
// RUN: %{cc1} %{diag} -fno-contracts-prove-assume -o %t.default.ll %s
// RUN: diff %t.ref.ll %t.default.ll
// RUN: %{cc1} %{diag} -fcontracts-prove-optimize=0 -o %t.level0.ll %s
// RUN: diff %t.ref.ll %t.level0.ll
// RUN: %{cc1} %{diag} -fno-contracts-prove-shadow -o %t.noshadow.ll %s
// RUN: diff %t.ref.ll %t.noshadow.ll
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -o %t.ignore.ll %s
// RUN: diff %t.ref.ll %t.ignore.ll
// RUN: %{cc1} %{diag} -o %t.assume.ll %s
// RUN: diff %t.ref.ll %t.assume.ll
// RUN: %{cc1} %{diag} -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -fcontracts-prove-assume -o %t.explicit0.ll %s
// RUN: diff %t.ref.ll %t.explicit0.ll
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E166): an -O0 module makes proof shadows at the proof level, 2 by
// default, and its code is still the same as at proof level 0, without the
// shadows and with every evaluation ignored, with every evaluation diagnose
// (and with -fcontracts-prove-assume, which keeps no assumption of these).
// -O0 removes nothing code generation leaves, so the assumption regions,
// the TBAA and !range made for the proofs and the allocas of the
// predicates' temporaries must leave no trace: here where a region is all
// a statement holds -- the arm of an `if', of a short-circuit condition, of
// a nested `if', a `switch' case, a loop body -- whatever the condition
// reads (parameters, memory, a call, a volatile object), where a predicate
// needs temporaries (a class with a destructor, a lambda), the function
// contract assertions, and loads of a `bool'.  With every strategy given
// explicitly at proof level 0 too, where the shadows run at level 0
// (DECISIONS.md E208 (a)).  Without value names: the regions' code takes
// names the real code's would otherwise have (the driver discards them).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-same-code-O0.C

struct S { int x; volatile int v; };
struct T { int n; T(int n) : n(n) {} ~T(); bool ok() const { return n > 0; } };
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
int f10(int n) { contract_assert(T(n).ok()); return 10; }
int f11(int n) { contract_assert([&] { return n > 0; }()); return 11; }
int f12(int i, int n) pre(i < n) post(r : r > 0) { if (i > 0) contract_assert(n > 1); return 12; }
int f13(int i, int n) { if (i < n) contract_assert(g(n) > 0); return 13; }
int f14(int i, int n) { while (i < n) { contract_assert(i < n); ++i; } return i; }
bool f15(bool b, int *p) { contract_assert(!b || *p > 0); return b; }
