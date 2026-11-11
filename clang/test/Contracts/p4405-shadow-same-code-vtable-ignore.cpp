// DEFINE: %{O} = -O1
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -triple x86_64-unknown-linux-gnu %{O}
// DEFINE: %{check} = %{cc1} -fcontract-evaluation-semantic=ignore -emit-llvm -o %t.ref.ll %s && \
// DEFINE:   %{cc1} -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -emit-llvm -o %t.ignore.ll %s && \
// DEFINE:   %{cc1} -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-llvm -o %t.shadow.ll %s && \
// DEFINE:   diff %t.ref.ll %t.ignore.ll && \
// DEFINE:   diff %t.ref.ll %t.shadow.ll && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ref.o %s && \
// DEFINE:   %{cc1} -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ignore.o %s && \
// DEFINE:   %{cc1} -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-obj -o %t.shadow.o %s && \
// DEFINE:   cmp %t.ref.o %t.ignore.o && \
// DEFINE:   cmp %t.ref.o %t.shadow.o
// RUN: %{check}
// REDEFINE: %{O} = -Og
// RUN: %{check}
// REDEFINE: %{O} = -O2
// RUN: %{check}
// REDEFINE: %{O} = -O0
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fdevirtualize-speculatively
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fstrict-vtable-pointers
// RUN: %{check}
// REQUIRES: x86-registered-target

// p4405-shadow-same-code-vtable.cpp without a proof shadow: every
// evaluation ignore, so there is no obligation and no shadow, and an
// ignore contract_assert emits nothing in the real code, its predicate's
// qualified call included, so V::ok and W::ok, inline virtual functions,
// are not emitted for it.  The real `v->ok(n)' and `w->ok(n)' stay
// virtual calls through V's and W's external vtables: the IR and the object
// code are the same with -fcontracts-p4405 as without it.  V::ok is
// declared pure, so its predicate has no side effects; W::ok's has the
// call's, which made it no premise before CLANG-918 was fixed.  The
// diagnose rows add a shadow, which now assumes W::ok's predicate after its
// obligation (the runtime evaluation is ignore) and V::ok's as before:
// still the same code.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-vtable-ignore.C
// (whose GCC-1000 gap, the real code folding to a definition only the
// assumption made, measured absent here before and after CLANG-918)

struct V { virtual int f(int); [[gnu::pure]] virtual bool ok(int n) const { return n > 0; } };
struct W { virtual int f(int); virtual bool ok(int n) const { return n > 0; } };

bool use(int n) {
  V *v = new V;
  if (n > 1)
    contract_assert(v->V::ok(n));
  return v->ok(n);
}

bool use2(int n) {
  W *w = new W;
  if (n > 1)
    contract_assert(w->W::ok(n));
  return w->ok(n);
}
