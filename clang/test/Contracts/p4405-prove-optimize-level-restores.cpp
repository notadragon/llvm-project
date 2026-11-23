// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only
// RUN: %{cc1} -O1 -verify %s
// RUN: %{cc1} -O3 -verify %s
// RUN: %{cc1} -Os -verify %s
// RUN: %{cc1} -Oz -verify %s
// RUN: %{cc1} -Oz -fcontracts-prove-optimize=follow -verify %s
// RUN: %{cc1} -Og -verify %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=follow -verify=expected,l1,nounroll %s
// RUN: %{cc1} -O2 -fcontracts-prove-optimize=1 -verify=expected,l1,nounroll %s
// RUN: %{cc1} -O1 -fcontracts-prove-optimize=follow -funroll-loops -verify=expected,l1 %s
// RUN: %{cc1} -O1 -fno-unroll-loops -verify=expected,nounroll %s
// RUN: %{cc1} -O2 -fno-unroll-loops -fcontracts-prove-optimize=3 -verify=expected,nounroll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O1 -emit-llvm -o - %s | FileCheck --implicit-check-not=clang.contract %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -Oz -emit-llvm -o - %s | FileCheck --implicit-check-not=clang.contract --check-prefix=OZ %s
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E165): the proofs are optimized at the proof level, by default the -O
// level but at least 2, with the settings that level implies, and what the
// -O level alone implies is replaced: -O1's llvm.loop.unroll.disable on
// every loop, and the optsize and minsize of -Os and -Oz.  So at -O1, -Os,
// -Oz and -Og these facts are proven as at -O2, which they are not at level
// 1 (follow at -O1, or =1): sum's (the -O1 pipeline does not prove it, nor
// does -Oz's minsize), loop2's (it needs the loop unrolled, which -O1 and
// -Os and -Oz prevent).  What the program asks for itself is followed: a
// function's own minsize (summ), a loop pragma (loop3), -fno-unroll-loops
// and -funroll-loops (loop2); sumc's own cold (optsize) does not matter.
// follow at -Oz is level 2.  The real code keeps its own settings, and no
// trace of the lists code generation records for the proof module.
//
// Mirror (results PO2-GCC should match): none yet.

int sum(const int *a, unsigned n) {
  int t = 0;
  for (unsigned j = 0; j < n; ++j)
    t += a[j];
  contract_assert(t != 1 || n > 0);  // l1-error {{contract assertion cannot be proven}}
  return t;
}

[[clang::minsize]] int summ(const int *a, unsigned n) {
  int t = 0;
  for (unsigned j = 0; j < n; ++j)
    t += a[j];
  contract_assert(t != 1 || n > 0);  // expected-error {{contract assertion cannot be proven}}
  return t;
}

[[gnu::cold]] int sumc(const int *a, unsigned n) {
  int t = 0;
  for (unsigned j = 0; j < n; ++j)
    t += a[j];
  contract_assert(t != 1 || n > 0);  // l1-error {{contract assertion cannot be proven}}
  return t;
}

bool cond(int);

int loop2() {
  int c = 0;
  for (int i = 0; i < 8; ++i)
    if (cond(i))
      ++c;
  contract_assert(c <= 8);           // nounroll-error {{contract assertion cannot be proven}}
  return c;
}

int loop3() {
  int c = 0;
#pragma nounroll
  for (int i = 0; i < 8; ++i)
    if (cond(i))
      ++c;
  contract_assert(c <= 8);           // expected-error {{contract assertion cannot be proven}}
  return c;
}

// The real code at -O1 keeps -O1's llvm.loop.unroll.disable; at -Oz, every
// function its minsize.
// CHECK: define {{.*}} @_Z3sumPKij(
// CHECK: !{!"llvm.loop.unroll.disable"}
// OZ: define {{.*}} @_Z3sumPKij({{.*}}) {{.*}}#[[A:[0-9]+]]
// OZ: attributes #[[A]] = {{{.*}}minsize{{.*}}}
