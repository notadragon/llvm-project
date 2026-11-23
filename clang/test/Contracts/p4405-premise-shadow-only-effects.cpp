// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-shadow-only-effects.json -emit-llvm -o - -Rcontract-proof -verify=force %s | FileCheck --check-prefix=FORCE %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-shadow-only-effects.json -emit-llvm -o - -Rcontract-proof -mllvm -contract-proof-force-inline=false -verify=noforce %s | FileCheck --check-prefix=NOFORCE %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-shadow-only-effects.json -emit-llvm -o - -Rcontract-proof -fno-contracts-prove-assume -verify=noassume %s | FileCheck --check-prefix=NOFORCE %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (M6-Clang): a callee inlined into the
// shadow but not into the real function (provoked as in
// p4405-premise-shadow-only.cpp), whose precondition this TU ignores, and a
// proof relying on what its code does not do: it calls only const functions, so
// the shadow sees that g is still 1 after the call.  For the inline callee
// another TU's definition, which the real function may call, may evaluate the
// precondition with observe, whose violation handler may store to g: the proof
// may rely on the callee's code only where the real function inlines it too, so
// Clang makes that call always_inline (FORCE): sound, assumed, the call gone --
// before M6-Clang the callee, holding no premise, was left out of line and the
// proof was sound and assumed all the same.  With
// -contract-proof-force-inline=false (NOFORCE) and under
// -fno-contracts-prove-assume (NOASSUME) it is conditional and the call stays.
// The static callee's copy is this TU's: sound, the call left alone.
// Runnable/p4405-premise-multi-tu-effects.cpp runs it.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-shadow-only-effects.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

[[gnu::const]] int sinkc (int);
int g;

namespace sem_ignore_inline {
inline int h_inline (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}
}

int g_inline (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  g = 1;
  if (x > 0) [[unlikely]]
    r = sem_ignore_inline::h_inline (y);
  int t = g;
  contract_assert (t == 1);  // force-remark {{proof shadow: proven, sound; assumed}} noforce-remark {{proof shadow: proven, conditional}} noassume-remark {{proof shadow: proven, conditional}}
  return r + (t == 1 ? 10 : 20);
}

namespace sem_ignore_static {
static int h_static (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}
}

int g_static (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  g = 1;
  if (x > 0) [[unlikely]]
    r = sem_ignore_static::h_static (y);
  int t = g;
  contract_assert (t == 1);  // force-remark {{proof shadow: proven, sound; assumed}} noforce-remark {{proof shadow: proven, sound; assumed}} noassume-remark {{proof shadow: proven, sound}}
  return r + (t == 1 ? 10 : 20);
}


// The forced call is inlined; every other call stays out of line.
// FORCE-LABEL: define {{.*}} @_Z8g_inlineii(
// FORCE-NOT: call {{.*}} @{{.*}}h_inline{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z8g_inlineii(
// NOFORCE: call {{.*}} @{{.*}}h_inline{{.*}}(
// FORCE-LABEL: define {{.*}} @_Z8g_staticii(
// FORCE: call {{.*}} @{{.*}}h_static{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z8g_staticii(
// NOFORCE: call {{.*}} @{{.*}}h_static{{.*}}(
