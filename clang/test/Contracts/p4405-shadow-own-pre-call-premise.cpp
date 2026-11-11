// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O0 -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Previous Assertions and Undefined Behavior" (CLANG-918, the
// former watch test): a function's own precondition whose predicate calls
// a function is a premise in its body.  The body's call of get, whose
// precondition is what each use's precondition says, is proven in use1
// (the predicate calls get, which has a precondition of its own), use2 (an
// inline accessor), use3 (the member read directly) and use4 (an ignore
// precondition).  Each own precondition's runtime evaluation is ignore
// (prove's, and ignore's), a premise the code does not guarantee, so the
// proofs are conditional (p4405-shadow-own-pre-call-premise-sound.cpp has
// sound ones).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-own-pre-call-premise.C

#include <contracts>
using namespace std::contracts::labels;

struct C {
  int *p;
  int *get() const pre<always_prove>(p) { return p; }
  int *raw() const { return p; }
};

void use1(C &c) pre<always_prove>(c.get() != nullptr) { c.get(); } // expected-remark {{proof shadow: proven, conditional}}
void use2(C &c) pre<always_prove>(c.raw() != nullptr) { c.get(); } // expected-remark {{proof shadow: proven, conditional}}
void use3(C &c) pre<always_prove>(c.p != nullptr) { c.get(); }     // expected-remark {{proof shadow: proven, conditional}}
void use4(C &c) pre<always_ignore>(c.raw() != nullptr) { c.get(); } // expected-remark {{proof shadow: proven, conditional}}
