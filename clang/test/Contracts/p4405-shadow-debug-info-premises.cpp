// DEFINE: %{O} = -O0
// DEFINE: %{cxx} = %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 --target=x86_64-unknown-linux-gnu %libcxx_flags -g -fno-contracts-prove-assume -fcontract-evaluation-semantic=observe -Wno-contract-unproven %{O} -c
// DEFINE: %{check} = %{cxx} -fno-contracts-prove-shadow -o %t.ref.o %s && \
// DEFINE:   %{cxx} -o %t.shadow.o %s && cmp %t.ref.o %t.shadow.o && \
// DEFINE:   %{cxx} -fno-contracts-prove-callee-premises -o %t.nocopies.o %s && cmp %t.ref.o %t.nocopies.o && \
// DEFINE:   %{cxx} -fcontracts-prove-optimize=0 -o %t.level0.o %s && cmp %t.ref.o %t.level0.o
// RUN: %{check}
// REDEFINE: %{O} = -Og
// RUN: %{check}
// REDEFINE: %{O} = -O1
// RUN: %{check}
// REDEFINE: %{O} = -O2
// RUN: %{check}
// REDEFINE: %{O} = -O3
// RUN: %{check}
// REQUIRES: contracts-libcxx, x86-registered-target

// D4405 proof shadows and -g: the premise copies
// (-fcontracts-prove-callee-premises, implied by -fcontracts-p4405) leave
// the debug information as it is without them, as the shadows do
// (p4405-shadow-debug-info.cpp).  p4405-shadow-callee-premises.cpp's shapes,
// under observe: each h's shadow decides its diagnose evaluation again with
// the premise copies of idx, idx2, k and take.  The object code, the debug
// sections included, is the same with them, without the copies, without
// any shadow and at proof level 0, at -O0, -Og/-O1 (proofs at level 2) and
// -O2/-O3.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-debug-info-premises.C

#include <contracts>
using namespace std::contracts::labels;

int idx(int x) post(r : r == 0) { return x; }
void h1(int x) {
  int i = idx(x);
  contract_assert<diagnose>(i == 0);
}

inline int idx2(int x) {
  contract_assert(x == 0);
  return x;
}
void h3(int x) {
  int i = idx2(x);
  contract_assert<diagnose>(i == 0);
}

int k(int x) { return idx(x) + 1; }
void h4(int x) {
  int i = k(x);
  contract_assert<diagnose>(i == 1);
}

void take(int x) pre(x == 0) {}
void h5(int x) {
  take(x);
  contract_assert<diagnose>(x == 0);
}
