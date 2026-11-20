// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fno-contracts-prove-assume
// RUN: %{cc1} -fmerge-functions -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -emit-llvm -o %t.diagnose.ll -Rcontract-proof %s 2> %t.merge.remarks
// RUN: %{cc1} -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -emit-llvm -o /dev/null -Rcontract-proof %s 2> %t.nomerge.remarks
// RUN: diff %t.merge.remarks %t.nomerge.remarks
// RUN: FileCheck %s --input-file=%t.merge.remarks
// RUN: %{cc1} -fmerge-functions -fcontract-analysis-semantic=ignore -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.diagnose.ll %t.ignore.ll
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", under -fmerge-functions: the
// predicate of D::f's precondition calls check3 and r, which have the same
// body, so D::f's override shadow uses both.  Its proof module does not
// merge them (it crashed the decision), so the remark is the one without
// -fmerge-functions, and the real module is the one every analysis
// evaluation ignore gives.

extern int sink(int);

__attribute__((noinline)) int r(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

__attribute__((noinline)) static int check3(int x) {
  return sink(x) + sink(x * 2) + sink(x * 3);
}

struct B {
  virtual int f(int x) pre(x > 0);
};

struct D : B {
  int f(int x) override pre(x >= 1 && check3(x) + r(x) > 0) { return r(x); }
};

void use() { D d; }

// CHECK: :32:7: remark: override shadow: not proven
