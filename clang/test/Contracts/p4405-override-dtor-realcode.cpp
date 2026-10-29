// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fno-contracts-prove-assume -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -O2 -mconstructor-aliases -emit-llvm -o %t.diagnose.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=ignore -fno-contracts-prove-assume -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -O2 -mconstructor-aliases -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.diagnose.ll %t.ignore.ll
// RUN: FileCheck %s --implicit-check-not=contract.override --implicit-check-not=__clang_contract_unproven --implicit-check-not=__clang_contract_proven --implicit-check-not=clang.contract. < %t.diagnose.ll
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E152: nothing of the override
// shadows of destructors -- user-provided and implicitly defined, of
// classes with and without virtual bases, with a throwing member
// destructor, with a precondition of their own -- reaches the real
// destructors, which keep their own checks (F::~F's precondition).  The
// output is the one every analysis evaluation ignore gives.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-dtor-realcode.C

int g;
void sink(int);

struct B {
  virtual ~B() noexcept(false) post(g == 1);
};

struct M { ~M() noexcept(false); int v; };

struct D : B {
  ~D() override { sink(1); }
};

struct E : B {
  M m;
};

struct F : B {
  ~F() override pre(g > 0) { sink(2); }
};

struct V : virtual B {
  int i;
};

struct W : V {
  ~W() override { for (int i = 0; i < g; ++i) sink(i); }
};

void kill(B *p) { delete p; }
void use() { D d; E e; F f; V v; W w; }

// CHECK-LABEL: define {{.*}} @_ZN1FD2Ev(
// CHECK: __cxa_contract_violation
