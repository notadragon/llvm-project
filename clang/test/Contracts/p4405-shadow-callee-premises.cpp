// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontracts-prove-callee-premises %libcxx_flags -O1 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fno-contracts-prove-callee-premises %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -O1 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -Og -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -Os -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=2 %libcxx_flags -Os -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 %libcxx_flags -O1 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontracts-prove-optimize=1 -fcontracts-prove-callee-premises %libcxx_flags -O1 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// REQUIRES: contracts-libcxx

// D4405R0 Section 4.5, notadragon_wg21 DECISIONS.md E196 item 2, E199 (b):
// an observe, noexcept_observe or ignore evaluation is a premise that makes
// a later proof conditional, and a conditional proof is not diagnosed --
// also when the evaluation is in a callee the proof shadow inlines
// (-fcontracts-prove-callee-premises, implied by -fcontracts-p4405).  The
// callee's premise copy, which assumes its observe and ignore evaluations,
// stands for it when the shadow decides again what it left unproven: a
// postcondition (h1), a contract_assert in an inline callee (h3), a
// precondition in the callee's body (h5), and a callee reached through a
// function without contract assertions the shadow inlines (h4: the copy
// stands for idx in k too).  h2's premise is in its own body, which the
// shadow assumes with or without the option.  With
// -fno-contracts-prove-callee-premises, h1, h3, h4 and h5 are errors, as
// before the option (CLANG-911).  At the proof optimization level
// (DECISIONS.md E162-E168, E205): the copies are made from level 2, the
// default at every -O level, so at -O1, -Og and -Os too the callees'
// premises reach the shadows; at level 1 they are not made (h1, h3, h4 and
// h5 are errors) unless -fcontracts-prove-callee-premises asks for them.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-callee-premises.C,
// p4405-prove-optimize-premises-O1.C, -premises-Og.C, -premises-Os.C

#include <contracts>
using namespace std::contracts::labels;

int idx(int x) post(r : r == 0) { return x; }
void h1(int x) {
  int i = idx(x);
  contract_assert<prove>(i == 0); // on-remark {{proof shadow: proven, conditional}} \
                                  // off-error {{cannot be proven}} \
                                  // off-remark {{proof shadow: not proven}}
}

void h2(int x) {
  contract_assert(x == 0);
  contract_assert<prove>(x == 0); // expected-remark {{proof shadow: proven, conditional}}
}

inline int idx2(int x) {
  contract_assert(x == 0);
  return x;
}
void h3(int x) {
  int i = idx2(x);
  contract_assert<prove>(i == 0); // on-remark {{proof shadow: proven, conditional}} \
                                  // off-error {{cannot be proven}} \
                                  // off-remark {{proof shadow: not proven}}
}

int k(int x) { return idx(x) + 1; }
void h4(int x) {
  int i = k(x);
  contract_assert<prove>(i == 1); // on-remark {{proof shadow: proven, conditional}} \
                                  // off-error {{cannot be proven}} \
                                  // off-remark {{proof shadow: not proven}}
}

void take(int x) pre(x == 0) {}
void h5(int x) {
  take(x);
  contract_assert<prove>(x == 0); // on-remark {{proof shadow: proven, conditional}} \
                                  // off-error {{cannot be proven}} \
                                  // off-remark {{proof shadow: not proven}}
}
