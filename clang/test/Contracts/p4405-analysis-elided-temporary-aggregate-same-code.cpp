// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fexceptions -fcxx-exceptions -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O0 -emit-llvm -o %t.diag0.ll -verify=diag %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fexceptions -fcxx-exceptions -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O0 -emit-llvm -o %t.ignore0.ll %s
// RUN: diff %t.diag0.ll %t.ignore0.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fexceptions -fcxx-exceptions -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.diag2.ll -verify=diag %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fexceptions -fcxx-exceptions -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o %t.ignore2.ll %s
// RUN: diff %t.diag2.ll %t.ignore2.ll
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fexceptions -fcxx-exceptions -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.diag2.o %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fexceptions -fcxx-exceptions -fcontract-evaluation-semantic=ignore -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.ignore2.o %s
// RUN: cmp %t.diag2.o %t.ignore2.o
// REQUIRES: x86-registered-target

// D4405 (notadragon_wg21 DECISIONS.md E160 (a)): the destructor call that
// would destroy an aggregate's already-built subobject is analyzed even
// when nothing after it can throw, and Clang emits no partial-destruction
// cleanup for it; the analysis emits nothing either.  With every
// evaluation diagnose (each analyzed, by folding without optimization and
// in a proof shadow with it, none proven), the module is emitted exactly
// as with every evaluation ignore, without optimization and with it.  The
// diagnostics themselves are p4405-analysis-elided-temporary-aggregate.cpp's.
// (Clang's own test.)

int g;
int nothr() noexcept;

struct DP {
  DP() noexcept;
  ~DP() pre(g > 0); // diag-note 8 {{the predicate of the contract assertion}}
};
struct A { DP p; int i; }; // diag-warning {{contract assertion cannot be proven}}
struct AI { DP p; int i = 5; }; // diag-warning {{contract assertion cannot be proven}}
struct D : DP { int i; ~D(); };

A e1{DP(), 1}; // diag-warning {{contract assertion cannot be proven}}

void f() {
  A b1{DP(), nothr()}; // diag-warning {{contract assertion cannot be proven}}
  static AI b2{DP()}; // diag-warning {{contract assertion cannot be proven}}
  D b3{{}, 1};
  DP b4[3] = {DP(), DP()};
  auto b5 = [p = DP(), i = 1] { return i; }; // diag-warning 2 {{contract assertion cannot be proven}}
  (void)b1;
  (void)b2;
  (void)b3;
  (void)b4;
  (void)b5();
} // diag-warning {{contract assertion cannot be proven}}
