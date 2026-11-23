// DEFINE: %{O} = -O0
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -debug-info-kind=constructor -dwarf-version=5 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %{O} -emit-obj
// DEFINE: %{check} = %{cc1} -fcontract-evaluation-semantic=ignore -o %t.ref.o %s && \
// DEFINE:   %{cc1} -o %t.shadow.o %s && cmp %t.ref.o %t.shadow.o && \
// DEFINE:   %{cc1} -fno-contracts-prove-shadow -o %t.noshadow.o %s && cmp %t.ref.o %t.noshadow.o && \
// DEFINE:   %{cc1} -fcontracts-prove-optimize=0 -o %t.level0.o %s && cmp %t.ref.o %t.level0.o
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

// D4405 proof shadows and -g: the analysis-only functions -- proof shadows
// and override shadows -- leave the debug information as it is without
// them.  A shadow is emitted without debug information, so neither the
// function it copies (f, f2, D::v, D2::get) nor a function called only in
// a predicate (g2, which the real f2 does not call) nor one the real code
// calls too (sq) is described differently, and an override shadow that
// needs a snapshot of a parameter the body modifies (B2::get's
// postcondition reads x, which D2::get assigns) leaves D2::get's local r
// in the function's outermost scope.  The object code, the debug sections
// included, is the same with the shadows, without them, at proof level 0
// and with every evaluation ignore, at -O0 (shadows at proof level 2),
// -Og/-O1 (level 2) and -O2/-O3.  The premise copies:
// p4405-shadow-debug-info-premises.cpp; statement shapes:
// p4405-shadow-debug-info-statements.cpp; what a predicate names:
// p4405-shadow-debug-info-types.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-debug-info.C,
// open-bug-p4405-shadow-debug-info-scope.C (D2::get, which differs in GCC)

int g2(int i) { return i + 1; }
static inline int sq(int i) { return i * i; }

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);
  return i;
}

int f2(int i, int n) {
  contract_assert(g2(i) > sq(n));
  return sq(i);
}

struct B { virtual int v(int x) pre(x > 0) post(r : r > 0); };
struct D : B {
  int v(const int x) override pre(x >= 0) post(r : r > x) { return x + 1; }
};
B *make() { return new D; }

int sink(int);
struct B2 { virtual int get(const int x) pre(x > 0) post(r : r >= x); };
struct D2 : B2 {
  int get(int x) override { int r = x; x = sink(x); return r + (x > 0); }
};
B2 *make2() { return new D2; }
