// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.observe.on.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fno-contracts-prove-callee-premises -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.observe.off.ll %s
// RUN: diff %t.observe.on.ll %t.observe.off.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.ignore.on.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fno-contracts-prove-callee-premises -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.ignore.off.ll %s
// RUN: diff %t.ignore.on.ll %t.ignore.off.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.observe.on.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fno-contracts-prove-callee-premises -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.observe.off.o %s
// RUN: cmp %t.observe.on.o %t.observe.off.o
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.ignore.on.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fno-contracts-prove-callee-premises -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.ignore.off.o %s
// RUN: cmp %t.ignore.on.o %t.ignore.off.o
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o /dev/null -Rcontract-proof %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o /dev/null -Rcontract-proof %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-configuration-file=%S/p4405-shadow-callee-premises-same-code.json -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fno-contracts-prove-callee-premises -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o /dev/null -Rcontract-proof %s 2>&1 | FileCheck %s --check-prefix=OFF
// REQUIRES: x86-registered-target

// D4405 (-fcontracts-prove-callee-premises), notadragon_wg21 DECISIONS.md
// E199 (b): the premise copies change nothing in the output.  The shadows
// are decided first with the real functions, as without the option, and the
// copies are used only in a second round for what that left unproven, whose
// proofs are only conditional; the copies, and a function only a copy uses
// (pred_only, used only in an ignored postcondition's predicate), are deleted
// with the shadows.  So the module is optimized and emitted exactly as with
// -fno-contracts-prove-callee-premises, also under -fcontracts-prove-assume
// (implied by -fcontracts-p4405), where sound proofs (s, whose own assertion
// is terminating by the configuration) are assumed in both,
// while h1, h3 and h4 are proven (conditionally) only with the option.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-callee-premises-same-code.C

int idx(int x) post(r : r == 0) { return x; }
int h1(int x) {
  int i = idx(x);
  contract_assert(i == 0);
  return i;
}

inline int idx2(int x) {
  contract_assert(x == 0);
  return x;
}
int h3(int x) {
  int i = idx2(x);
  contract_assert(i == 0);
  return i * 2;
}

int k(int x) { return idx(x) + 1; }
int h4(int x) {
  int i = k(x);
  contract_assert(i == 1);
  return i;
}

inline bool pred_only(int r) { return r < 100; }
int idx3(int x) post(r : pred_only(r)) { return x; }
int h6(int x) { return idx3(x) + idx3(x + 1); }

namespace sure {
int s(int x) {
  if (x != 0)
    __builtin_unreachable();
  int i = idx(x);
  contract_assert(i == 0);
  return i == 0;
}
} // namespace sure

// The analysis evaluations of h1's, h3's and h4's contract_assert: proven
// conditionally with the option, not proven without it.
// ON: :34:3: remark: proof shadow: proven, conditional
// ON: :44:3: remark: proof shadow: proven, conditional
// ON: :51:3: remark: proof shadow: proven, conditional
// ON: :64:3: remark: proof shadow: proven, sound; assumed
// OFF: :34:3: remark: proof shadow: not proven
// OFF: :44:3: remark: proof shadow: not proven
// OFF: :51:3: remark: proof shadow: not proven
// OFF: :64:3: remark: proof shadow: proven, sound; assumed
