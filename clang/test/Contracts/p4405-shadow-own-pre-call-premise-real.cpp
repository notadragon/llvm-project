// DEFINE: %{O} = -O2
// DEFINE: %{cxx} = %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags --target=x86_64-unknown-linux-gnu
// DEFINE: %{check} = %{cxx} %{O} -fno-contracts-prove-assume -S -emit-llvm -o %t.ll -Rcontract-proof -Xclang -verify %s && \
// DEFINE:   FileCheck %s --input-file=%t.ll && \
// DEFINE:   %{cxx} %{O} -DREF -S -emit-llvm -o %t.ref.ll %s && \
// DEFINE:   diff %t.ref.ll %t.ll && \
// DEFINE:   %{cxx} %{O} -fno-contracts-prove-assume -c -o %t.o %s && \
// DEFINE:   %{cxx} %{O} -DREF -c -o %t.ref.o %s && \
// DEFINE:   cmp %t.ref.o %t.o
// RUN: %{check}
// REDEFINE: %{O} = -O1
// RUN: %{check}
// REDEFINE: %{O} = -O0
// RUN: %{check}
// REQUIRES: contracts-libcxx, x86-registered-target

// CLANG-918: an assumption whose predicate calls functions leaves the real
// code unchanged.  The contract_assert evaluations are ignore; f's shadow
// assumes their predicates, which call an opaque function and an inline
// accessor that is never inlined, and proves the prove evaluation after
// them conditionally.  The real code never evaluates those predicates, so
// neither function is called, nor is the accessor emitted, and with
// -fno-contracts-prove-assume the IR and the object code are those of the
// same function without the prove evaluation (-DREF: every evaluation
// ignore, so no shadow is made).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-own-pre-call-premise-real.C

#include <contracts>
using namespace std::contracts::labels;

int sink(int);
bool opaque(int);
[[gnu::noinline]] inline bool positive(int x) { return x > 0; }

int f(int x) {
  contract_assert(opaque(x) && x > 1);
  contract_assert(positive(x));
#ifdef REF
  contract_assert(x > 1);
#else
  contract_assert<prove>(x > 1); // expected-remark-re {{proof shadow: proven, conditional{{$}}}}
#endif
  return sink(x);
}

// CHECK-NOT: opaque
// CHECK-NOT: positive
