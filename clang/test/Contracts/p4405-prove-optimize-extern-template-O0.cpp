// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0
// RUN: %{cc1} -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify=on %s
// RUN: %{cc1} -fcontract-evaluation-semantic=diagnose -O2 -emit-llvm-only -verify=on %s
// RUN: %{cc1} -fcontract-evaluation-semantic=diagnose -fcontracts-prove-optimize=0 -emit-llvm-only -verify=off %s
// RUN: %{cc1} -fcontract-evaluation-semantic=diagnose -fno-contracts-prove-assume -emit-llvm -o %t.on.ll %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0 -emit-llvm -o %t.ref.ll %s
// RUN: diff %t.ref.ll %t.on.ll
// RUN: FileCheck %s --input-file=%t.on.ll
// REQUIRES: x86-registered-target

// D4405's proof optimization level (notadragon_wg21 DECISIONS.md E162-E168,
// E166): in an -O0 module, at the default proof level, 2, the proof shadows
// use the bodies of the inline members of a class template whose
// instantiation is declared by `extern template' -- available_externally
// functions, whose bodies -O0 code generation otherwise never emits -- so
// f's assertion, which needs S<int>::fix inlined, is proven as at -O2.  The
// bodies are emitted for the proofs only: S<int>::fix stays a declaration,
// helper<int>, which only it calls, is not emitted, and the code is the
// same as at proof level 0 (where the assertion is not proven) with
// ignore.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-optimize-extern-template-O0.C

template <class T> inline T helper(T v) { return v < 0 ? 0 : v; }
template <class T> struct S { void fix(T &); };
template <class T> inline void S<T>::fix(T &v) { v = helper(v); }
extern template struct S<int>;

void f(int v) {
  S<int> s;
  s.fix(v);
  contract_assert(v >= 0); // off-warning {{contract assertion cannot be proven}}
}

// on-no-diagnostics
// CHECK: declare void @_ZN1SIiE3fixERi(
// CHECK-NOT: define {{.*}}@_ZN1SIiE3fixERi(
// CHECK-NOT: _Z6helperIiET_S0_
