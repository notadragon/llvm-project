// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2
// DEFINE: %{shadow} = -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven
// RUN: %{cc1} -fmerge-functions %{shadow} -emit-llvm -o %t.shadow.ll -Rcontract-proof %s 2> %t.merge.remarks
// RUN: %{cc1} %{shadow} -emit-llvm -o /dev/null -Rcontract-proof %s 2> %t.nomerge.remarks
// RUN: diff %t.merge.remarks %t.nomerge.remarks
// RUN: FileCheck %s --check-prefix=REMARK --input-file=%t.merge.remarks
// RUN: %{cc1} -fmerge-functions %{shadow} -fno-contracts-prove-shadow -emit-llvm -o %t.noshadow.ll %s
// RUN: %{cc1} -fmerge-functions -fcontract-evaluation-semantic=ignore -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.shadow.ll %t.noshadow.ll
// RUN: diff %t.shadow.ll %t.ignore.ll
// RUN: %{cc1} -fmerge-functions %{shadow} -emit-obj -o %t.shadow.o %s
// RUN: %{cc1} -fmerge-functions %{shadow} -fno-contracts-prove-shadow -emit-obj -o %t.noshadow.o %s
// RUN: cmp %t.shadow.o %t.noshadow.o
// REQUIRES: x86-registered-target

// P4405 proof shadow under -fmerge-functions: check, which only
// the predicate of f's assertion calls (so only f's shadow), has the body of
// r.  The proof module is optimized without identical-function merging,
// which would replace check there by a new thunk to r while the decision
// still held check (a crash in the decision), so the remarks are those
// without -fmerge-functions, and the real module, its IR and its object
// code, are exactly what they are without the shadows and with every
// evaluation ignore.  The control without -fmerge-functions is
// p4405-shadow-same-code-icf.cpp; the premise round and the override shadows
// are p4405-shadow-merge-functions-premise.cpp and
// p4405-shadow-merge-functions-override.cpp.

extern int sink(int);

__attribute__((noinline)) static int check(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

__attribute__((noinline)) int r(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

int f(int n) {
  contract_assert(check(n) > 0);
  return r(n);
}

// REMARK-COUNT-1: remark: proof shadow: not proven
