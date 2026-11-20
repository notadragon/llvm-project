// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -disable-llvm-passes -emit-llvm-bc -o %t.bc -verify %s
// RUN: llvm-dis %t.bc -o - | FileCheck %s --implicit-check-not=contract.proof --implicit-check-not=__clang_contract --implicit-check-not=clang.contract --implicit-check-not=llvm.assume
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -O2 -save-temps -c %s 2>&1 | FileCheck --check-prefix=DRIVER %s
// REQUIRES: x86-registered-target

// P4405 stage 3: with the LLVM passes disabled at -O2 -- as -save-temps
// does for the step that writes the bitcode -- the proof shadows are still
// made and decided at the -O level, so the same evaluations are proven as
// without -save-temps; the module written is unoptimized, without any
// shadow, and keeps no assumption (nothing would remove it again).
// (Clang's own test.)

// DRIVER: "-disable-llvm-passes"

// CHECK: define {{.*}} @_Z1fii(

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);
  contract_assert(i < n);       // expected-warning {{contract assertion cannot be proven}}
  return i;
}
