// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2
// DEFINE: %{shadow} = -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven
// RUN: %{cc1} %{shadow} -emit-llvm -o %t.shadow.ll -Rcontract-proof %s 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: %{cc1} %{shadow} -fno-contracts-prove-shadow -emit-llvm -o %t.noshadow.ll %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.shadow.ll %t.noshadow.ll
// RUN: diff %t.shadow.ll %t.ignore.ll
// RUN: FileCheck %s --input-file=%t.shadow.ll
// RUN: %{cc1} %{shadow} -S -o %t.shadow.s %s
// RUN: %{cc1} %{shadow} -fno-contracts-prove-shadow -S -o %t.noshadow.s %s
// RUN: %{cc1} -fcontract-evaluation-semantic=ignore -S -o %t.ignore.s %s
// RUN: diff %t.shadow.s %t.noshadow.s
// RUN: diff %t.shadow.s %t.ignore.s
// REQUIRES: x86-registered-target

// P4405: a proof shadow takes no name or number in the real module, so the
// labels and names of the real code are as without it: f, which holds an
// unproven evaluation, is inlined into k1 and k2.  The IR and the assembly,
// not only the object code, as the label numbers are not in the object, are
// exactly what they are without the shadows (-fno-contracts-prove-shadow)
// and with every evaluation ignore; -fno-contracts-prove-assume, so that
// nothing proven is assumed.  Clang's shadows live in a proof module of their
// own.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-numbering.C
// (GCC-907).

extern int sink(int);
extern int cold_path(int);

int f(int n) {
  contract_assert(n > 0);
  if (n > 1000) {
    int s = 0;
    for (int i = 0; i < n; ++i)
      s += sink(i) * sink(i + 1) + cold_path(s);
    for (int i = 0; i < n; ++i)
      s ^= sink(s + i);
    return s;
  }
  return n + 1;
}

int k1(int n) { return f(n) + f(n + 1); }
int k2(int n) { return f(n * 2); }

// REMARK-COUNT-1: remark: proof shadow: not proven

// CHECK: define {{.*}} @_Z1fi(
// CHECK: define {{.*}} @_Z2k1i(
