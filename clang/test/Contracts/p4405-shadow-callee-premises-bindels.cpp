// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=diagnose -Wno-contract-unproven %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=diagnose -Wno-contract-unproven %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose -Wno-contract-unproven %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose -Wno-contract-unproven %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fno-contracts-prove-callee-premises %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fno-contracts-prove-callee-premises %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,off %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fno-contracts-prove-callee-premises %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fno-contracts-prove-callee-premises %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Xclang -verify=expected,on %s
// REQUIRES: contracts-libcxx

// D4405R0's Peter Bindels `expected` example (the celink
// D4405R0-implexample-bindels-expected, with the paper's `proven` label
// spelled as the standard `prove`), D4405R0 Section 4.5, notadragon_wg21
// DECISIONS.md E196 item 2, E199 (b).  The call of value() in the else
// branch relies on the postcondition of index(): proven soundly when that
// is checked and terminating (enforce, quick_enforce), and conditionally,
// so not diagnosed, when it is observed or ignored -- through index()'s
// premise copy (-fcontracts-prove-callee-premises, implied by
// -fcontracts-p4405).  With -fno-contracts-prove-callee-premises the
// observed or ignored postcondition is lost in the inlined index(), and the
// call is an error (CLANG-911).  The if branch needs no premise.  Also with
// every assertion analyzed (-fcontract-analysis-semantic=diagnose), where an
// ignored postcondition after its own analysis evaluation is assumed in the
// copy, though a shadow leaves it to that evaluation's obligation.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-callee-premises-bindels.C

#include <contracts>
using namespace std::contracts::labels;

struct Expected {
  int d_value;
  int d_error;
  int d_index;

  int index() const post(r : r == 0 || r == 1) { return d_index; }
  bool is_error() const { return index() == 1; }
  int error() const pre<prove>(index() == 1) { return d_error; }
  int value() const pre<prove>(index() == 0) { return d_value; } // off-note {{predicate}}
};

Expected get12thPrime();
void use(int);

void g()
{
  Expected p = get12thPrime();
  if (p.is_error()) {
    use(p.error());  // OK, proven
  }
  else {
    use(p.value());  // off-error {{cannot be proven}}
  }
}

// on-no-diagnostics
