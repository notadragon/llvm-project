// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=diagnose -Wno-contract-unproven
// RUN: %{cc1} -fmerge-functions -emit-llvm -o %t.on.ll -Rcontract-proof %s 2> %t.merge.remarks
// RUN: %{cc1} -emit-llvm -o /dev/null -Rcontract-proof %s 2> %t.nomerge.remarks
// RUN: diff %t.merge.remarks %t.nomerge.remarks
// RUN: FileCheck %s --input-file=%t.merge.remarks
// RUN: %{cc1} -fmerge-functions -fno-contracts-prove-callee-premises -emit-llvm -o %t.off.ll %s
// RUN: diff %t.on.ll %t.off.ll
// REQUIRES: x86-registered-target

// D4405 (-fcontracts-prove-callee-premises) under -fmerge-functions:
// h1's shadow uses idx, so the premise round decides it again
// with idx's premise copy, and both rounds' proof modules hold check2, which
// only the predicate of h1's second assertion calls, and r, which has the
// same body.  Neither round merges them (it crashed the decision), so the
// remarks are those without -fmerge-functions, and the real module is the
// same with the premise copies and without.

extern int sink(int);

__attribute__((noinline)) int r(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

__attribute__((noinline)) static int check2(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

int idx(int x) post(v : v == 0) { return x; }

int h1(int x) {
  int i = idx(x);
  contract_assert(i == 0);
  contract_assert(check2(x) > 0);
  return r(i);
}

// CHECK: :28:16: remark: proof shadow: not proven
// CHECK: :32:3: remark: proof shadow: proven, conditional
// CHECK: :33:3: remark: proof shadow: not proven
