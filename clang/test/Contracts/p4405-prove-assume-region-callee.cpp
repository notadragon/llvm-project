// DEFINE: %{O} = -O2
// DEFINE: %{cc1} = %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu %{O} -Wno-contract-unproven
// DEFINE: %{check} = %{cc1} -fcontract-evaluation-semantic=ignore -emit-obj -o %t.ref.o %s && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=diagnose -emit-obj -o %t.assume.o %s && cmp %t.ref.o %t.assume.o && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=diagnose -fcontracts-prove-optimize=0 -emit-obj -o %t.level0.o %s && cmp %t.ref.o %t.level0.o && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=diagnose -fno-contracts-prove-assume -emit-obj -o %t.noassume.o %s && cmp %t.ref.o %t.noassume.o && \
// DEFINE:   %{cc1} -fcontract-evaluation-semantic=diagnose -emit-llvm -o - %s | FileCheck %s
// RUN: %{check}
// REDEFINE: %{O} = -O0
// RUN: %{check}
// REDEFINE: %{O} = -O0 -fno-inline
// RUN: %{check}
// REDEFINE: %{O} = -O1
// RUN: %{check}
// REDEFINE: %{O} = -O1 -fno-inline
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fno-inline
// RUN: %{check}
// REDEFINE: %{O} = -O2 -fno-inline-functions
// RUN: %{check}
// REDEFINE: %{O} = -O3
// RUN: %{check}
// REDEFINE: %{O} = -O3 -fno-inline-functions
// RUN: %{check}
// REDEFINE: %{O} = -Os
// RUN: %{check}
// REDEFINE: %{O} = -Os -fno-inline
// RUN: %{check}
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0 -DPROVEN -fcontract-evaluation-semantic=diagnose -emit-llvm -o - %s | FileCheck %s --check-prefix=PROVEN
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -DPROVEN -fcontract-evaluation-semantic=diagnose -emit-llvm -o - %s | FileCheck %s --check-prefix=PROVEN
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fno-inline -DPROVEN -fcontract-evaluation-semantic=diagnose -emit-llvm -o - %s | FileCheck %s --check-prefix=PROVEN
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O0 -DPROVEN -fcontract-evaluation-semantic=diagnose -Rcontract-proof -emit-llvm-only -verify=proven %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fno-inline -DPROVEN -fcontract-evaluation-semantic=diagnose -Rcontract-proof -emit-llvm-only -verify=proven %s
// REQUIRES: x86-registered-target

// D4405 under -fcontracts-prove-assume (on with -fcontracts-p4405): a
// function that only an unproven obligation's predicate calls, and that is
// not inlined, is not emitted.  The real function's assumption region for
// each obligation is deleted once the proofs are decided, before the
// optimization pipeline, with every internal, linkonce or
// available_externally definition only it used, so h (inline, noinline),
// s (static, noinline), the static s3 and s4 only s3 calls, the template t,
// the lambda's call operator, the inline virtual V::ok called by its
// qualified name and the caller-side precondition's h are not emitted, at
// any level (-O0 included) and with -fno-inline or -fno-inline-functions:
// the object code is the same as with every evaluation ignore, at proof
// level 0, and without -fcontracts-prove-assume.  With PROVEN, the
// obligations are proven soundly and their assumptions kept; the
// functions their predicates call are not emitted either (the code around
// a kept region may differ, DECISIONS.md E167, so only that is checked).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-prove-assume-region-callee.C
// in the gnu_gcc fork (GCC-1001).

extern int g(int);

#ifndef PROVEN
[[gnu::noinline]] inline bool h(int n) { return g(n) > 0; }
[[gnu::noinline]] static bool s(int n) { return g(n + 1) > 0; }
inline bool h2(int n) { return g(n + 2) > 0; }
static bool s2(int n) { return g(n + 3) > 0; }
[[gnu::noinline]] static bool s4(int n) { return g(n + 4) > 0; }
[[gnu::noinline]] static bool s3(int n) { return s4(n) && g(n + 5) > 0; }
template <class T> [[gnu::noinline]] bool t(T n) { return g(n + 6) > 0; }
struct V { virtual int f(int); virtual bool ok(int n) const { return g(n + 7) > 0; } };

int f(int n) {
  contract_assert(h(n));
  contract_assert(s(n));
  contract_assert(h2(n));
  contract_assert(s2(n));
  contract_assert(s3(n));
  contract_assert(t(n));
  contract_assert([&] { return g(n + 8) > 0; }());
  return n;
}

bool vu(V *v, int n) {
  contract_assert(v->V::ok(n));
  return n > 0;
}

int k(int n) pre(h(n + 9));
int m(int n) { return k(n); }

// CHECK-NOT: define {{.*}}@_Z1hi(
// CHECK-NOT: define {{.*}}@_ZL1si(
// CHECK-NOT: define {{.*}}@_Z2h2i(
// CHECK-NOT: define {{.*}}@_ZL2s2i(
// CHECK-NOT: define {{.*}}@_ZL2s3i(
// CHECK-NOT: define {{.*}}@_ZL2s4i(
// CHECK-NOT: define {{.*}}@_Z1tIiEbT_(
// CHECK-NOT: define {{.*}}@"_ZZ1fiENK3$_0clEv"(
// CHECK-NOT: define {{.*}}@_ZNK1V2okEi(
// CHECK: define {{.*}}@_Z1fi(
// CHECK-NOT: define {{.*}}@_Z1hi(
// CHECK-NOT: define {{.*}}@_ZL1si(
// CHECK-NOT: define {{.*}}@_Z2h2i(
// CHECK-NOT: define {{.*}}@_ZL2s2i(
// CHECK-NOT: define {{.*}}@_ZL2s3i(
// CHECK-NOT: define {{.*}}@_ZL2s4i(
// CHECK-NOT: define {{.*}}@_Z1tIiEbT_(
// CHECK-NOT: define {{.*}}@"_ZZ1fiENK3$_0clEv"(
// CHECK-NOT: define {{.*}}@_ZNK1V2okEi(
// CHECK: define {{.*}}@_Z2vuP1Vi(
// CHECK-NOT: define {{.*}}@_Z1hi(
// CHECK-NOT: define {{.*}}@_ZNK1V2okEi(
// CHECK: define {{.*}}@_Z1mi(
// CHECK-NOT: define {{.*}}@_Z1hi(
#else
[[gnu::noinline]] static bool ps(int) { return true; }
[[gnu::noinline]] inline bool ph(int n) { return n > 0; }
static bool ps2(int) { return true; }
inline bool ph2(int n) { return n > 1; }

int f1(int n) {
  contract_assert(ps(n)); // proven-remark {{proof shadow: proven, sound; assumed}}
  return n;
}
int f2(int n) {
  if (n <= 0) return 0;
  contract_assert(n > 0 || ph(n)); // proven-remark {{proof shadow: proven, sound; assumed}}
  return n;
}
int f3(int n) {
  contract_assert(ps2(n)); // proven-remark {{proof shadow: proven, sound; assumed}}
  return n;
}
int f4(int n) {
  if (n <= 0) return 0;
  contract_assert(n > 0 || ph2(n)); // proven-remark {{proof shadow: proven, sound; assumed}}
  return n;
}

// PROVEN-NOT: define {{.*}}@_ZL2psi(
// PROVEN-NOT: define {{.*}}@_Z2phi(
// PROVEN-NOT: define {{.*}}@_ZL3ps2i(
// PROVEN-NOT: define {{.*}}@_Z3ph2i(
// PROVEN: define {{.*}}@_Z2f1i(
// PROVEN-NOT: define {{.*}}@_ZL2psi(
// PROVEN-NOT: define {{.*}}@_Z2phi(
// PROVEN-NOT: define {{.*}}@_ZL3ps2i(
// PROVEN-NOT: define {{.*}}@_Z3ph2i(
// PROVEN: define {{.*}}@_Z2f4i(
// PROVEN-NOT: define {{.*}}@_ZL2psi(
// PROVEN-NOT: define {{.*}}@_Z2phi(
// PROVEN-NOT: define {{.*}}@_ZL3ps2i(
// PROVEN-NOT: define {{.*}}@_Z3ph2i(
#endif
