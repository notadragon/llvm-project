// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,on %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fno-contracts-prove-callee-premises %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify=expected,off %s
// REQUIRES: contracts-libcxx

// CLANG-918 with -fcontracts-prove-callee-premises (implied by
// -fcontracts-p4405): an observe or ignore precondition whose predicate
// calls a function is a premise in its function's premise copy as well, so
// a caller's shadow that calls the copy proves conditionally what only that
// precondition gives -- never an assumption (no "assumed" in the remarks).
// take1's predicate calls an inline accessor, take2's an opaque function.
// h1's and h2's evaluations are left unproven by the ordinary shadow (an
// error with -fno-contracts-prove-callee-premises) and proven,
// conditionally, by the premise round.  Clang's observe premise copies
// assumed such a predicate already (an evaluated predicate's calls are made
// by the real code too); the ignore runs are the ones CLANG-918 changed.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-own-pre-call-premise-callee-premises.C

#include <contracts>
using namespace std::contracts::labels;

bool opaque(int);

struct C {
  int n;
  int get() const { return n; }
};

void take1(const C &c) pre(c.get() > 0) {}
void h1(C &c) {
  take1(c);
  contract_assert<prove>(c.n > 0); // on-remark-re {{proof shadow: proven, conditional{{$}}}} \
                                   // off-error {{cannot be proven}} \
                                   // off-remark {{proof shadow: not proven}}
}

void take2(int x) pre(opaque(x) && x > 0) {}
void h2(int x) {
  take2(x);
  contract_assert<prove>(x > 0); // on-remark-re {{proof shadow: proven, conditional{{$}}}} \
                                 // off-error {{cannot be proven}} \
                                 // off-remark {{proof shadow: not proven}}
}
