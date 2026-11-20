// DEFINE: %{O} = -O1
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu %{O}
// DEFINE: %{check} = %{cc1} -fcontract-evaluation-semantic=ignore -emit-llvm -o %t.ref.ll %s && \
// DEFINE:   %{cc1} -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-llvm -o %t.shadow.ll %s && \
// DEFINE:   diff %t.ref.ll %t.shadow.ll && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ref.o %s && \
// DEFINE:   %{cc1} -fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -emit-obj -o %t.shadow.o %s && \
// DEFINE:   cmp %t.ref.o %t.shadow.o
// RUN: %{check}
// REDEFINE: %{O} = -O2
// RUN: %{check}
// REDEFINE: %{O} = -O3
// RUN: %{check}
// REDEFINE: %{O} = -O0
// RUN: %{check}
// REDEFINE: %{O} = -O0 -fdevirtualize-speculatively
// RUN: %{check}
// REDEFINE: %{O} = -O1 -fdevirtualize-speculatively
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fdevirtualize-speculatively
// RUN: %{check}
// REDEFINE: %{O} = -O3 -fdevirtualize-speculatively
// RUN: %{check}
// REDEFINE: %{O} = -Os -fdevirtualize-speculatively
// RUN: %{check}
// REDEFINE: %{O} = -O1 -fstrict-vtable-pointers
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fstrict-vtable-pointers
// RUN: %{check}
// REDEFINE: %{O} = -O3 -fstrict-vtable-pointers
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fdevirtualize-speculatively -fstrict-vtable-pointers
// RUN: %{check}
// REQUIRES: x86-registered-target

// D4405 proof shadows change nothing in the real code, through a vtable
// either (CLANG-914).  V's key function is defined elsewhere, so its
// vtable is emitted here only speculatively (available_externally), and
// only if every inline virtual function of V has been emitted.  Nothing in
// the real code emits V::ok; the predicate's qualified call does, in use's
// proof shadow.  The speculative vtables are emitted before the shadows,
// so V's is not, and use's real `v->ok(n)' stays a virtual call: the IR and
// the object code are the same with the shadows as with every evaluation
// ignore, at -O1, -O2, -O3 (and -O0, which emits no speculative vtable).
//
// GCC's mirror also compares with -fno-devirtualize; Clang accepts that
// flag only to ignore it (a GCC optimization flag it does not implement)
// and has no switch that turns devirtualization off.  Its vtable controls
// opt in to more: -fdevirtualize-speculatively gives the vtable type
// metadata and keeps it (llvm.compiler.used, even when it is only
// available_externally) and adds type tests to the virtual calls, and
// -fstrict-vtable-pointers marks the vtable loads invariant (also at
// -O0 for the first, only above -O0 for the second).  Both change this
// test's IR, so they are the rows here: the shadows still change neither
// the IR nor the object code.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-same-code-vtable.C
// in the gnu_gcc fork (GCC-1000).

struct V { virtual int f(int); virtual bool ok(int n) const { return n > 0; } };

bool use(int n) {
  V *v = new V;
  if (n > 1)
    contract_assert(v->V::ok(n));
  return v->ok(n);
}
