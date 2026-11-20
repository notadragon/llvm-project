// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only -round-trip-args
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -verify=noshadow,nooverride %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-assume -verify=noshadow,nooverride %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -verify=noshadow,nooverride %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=1 -verify=nooverride %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=1 -fcontracts-prove-override -verify=all %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=2 -verify=all %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=3 -verify=all %s
// RUN: %{cc1} -O3 -fcontracts-prove-optimize=3 -fno-contracts-prove-override -fno-contracts-prove-assume -verify=nooverride %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-optimize=2 -verify=all %s
// RUN: %{cc1} -O1 -verify=all %s
// RUN: %{cc1} -O3 -verify=all %s
// RUN: %{cc1} -Og -verify=all %s
// RUN: %{cc1} -Os -verify=all %s
// RUN: %{cc1} -Oz -verify=all %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=follow -verify=nooverride %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=follow -verify=all %s
// RUN: %{cc1} -Og -fcontracts-prove-optimize=follow -verify=all %s
// RUN: %{cc1} -Os -fcontracts-prove-optimize=follow -verify=all %s
// RUN: %{cc1} -Oz -fcontracts-prove-optimize=follow -verify=all %s
// RUN: %{cc1} -O0 -verify=all %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=follow -verify=noshadow,nooverride %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=1 -verify=nooverride %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -verify=noshadow,nooverride %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=0 -verify=noshadow,nooverride %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=1 -fcontracts-prove-override -verify=all %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=2 -verify=all %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=3 -verify=all %s
// RUN: %{cc1} -O0 -fcontracts-prove-optimize=3 -fno-contracts-prove-override -fno-contracts-prove-assume -verify=nooverride %s
// RUN: %clang -std=c++26 -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove --target=x86_64-unknown-linux-gnu -O1 -fcontracts-prove-optimize=follow -S -emit-llvm -o %t.ll -Xclang -verify=nooverride %s
// RUN: %clang -std=c++26 -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove --target=x86_64-unknown-linux-gnu -O1 -S -emit-llvm -o %t.ll -Xclang -verify=all %s
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E205), -fcontracts-prove-optimize=<0|1|2|3|follow>: each proving strategy
// is on from a proof level -- folding from 0, the proof shadow and assume
// from 1, the override shadows from 2 -- and one given explicitly, in either
// form, wins.  By default the level is the -O level, but at least 2 (-O3
// gives 3); follow is the -O level, where -Os, -Oz and -Og count as 2; the
// last option wins.  D::f is proven by its override shadow, from B::f's
// precondition (below level 2 its pair takes the -fno-contracts-prove-override
// fallback, E157); D::g and k by their proof shadows; D::h by folding.  At
// -O0 too the shadows are made at the proof level, 2 by default (stage 2,
// PO5), each level and explicit form giving the diagnostics it gives in an
// optimized build.  -round-trip-args checks that the front end's resolved level and
// strategies survive the arguments' regeneration.  A strategy given
// explicitly at level 0 runs at level 0 (DECISIONS.md E208 (a)), where the
// shadows, optimized as at -O0, prove none of these.  (Which strategies
// become assumptions: p4405-prove-optimize-assume.cpp.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-0.C,
// -0-explicit.C, -0-shadow.C, -1.C, -1-override.C, -2.C, -3.C,
// -3-explicit-off.C, -last.C, -default-O1.C, -default-O3.C, -default-Og.C,
// -default-Os.C, -default-Oz.C, -follow-O1.C, -follow-O2.C, -follow-Og.C,
// -follow-Os.C, -follow-Oz.C

struct B {
  virtual int f(int x) pre(x > 0) post(r: r > 0);  // nooverride-note {{the predicate of the contract assertion}}
  virtual int g(int x) pre(x > 0) post(r: r > 0);  // noshadow-note {{the predicate of the contract assertion}}
  virtual int h(int x) pre(x > 0);
};

struct D : B {
  // Proven in an override shadow, from B::f's precondition.
  int f(int x) override        // nooverride-error {{cannot be proven for a call through 'B::f'}}
  { return x; }
  // Proven in D::g's proof shadow.
  int g(int x) override        // noshadow-error {{cannot be proven for a call through 'B::g'}}
  { return x > 0 ? x : 1; }
  // A tautology, proven by folding.
  int h(int x) override pre(true) { return x; }
};

void use() { D d; }

// Proven in k's proof shadow.
int k(int i, int n) {
  if (i < n)
    contract_assert(i < n);    // noshadow-error {{contract assertion cannot be proven}}
  return i;
}

// all-no-diagnostics
