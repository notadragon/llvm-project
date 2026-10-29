// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-shadow-only.json -emit-llvm -o - -Rcontract-proof -verify=force %s | FileCheck --check-prefix=FORCE %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-shadow-only.json -emit-llvm -o - -Rcontract-proof -mllvm -contract-proof-force-inline=false -verify=noforce %s | FileCheck --check-prefix=NOFORCE %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-shadow-only.json -emit-llvm -o - -Rcontract-proof -fno-contracts-prove-assume -verify=noassume %s | FileCheck --check-prefix=NOFORCE %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): a callee inlined into the
// shadow but not into the real function.  In each g_* the call is in a branch
// marked [[unlikely]], so the inliner's cold call-site threshold keeps the
// callee out of line in the real function, while in the shadow an earlier
// assume evaluation (an own-body premise; the real code lowers assume like
// ignore, CLANG-111) decides the branch, so the call is not cold and is
// inlined.  An inline or template callee's enforce precondition is decided here
// only if the real function inlines it too, so Clang makes that call
// always_inline in the real function (FORCE): sound, assumed, and the real g_*
// no longer calls the callee.  With -contract-proof-force-inline=false
// (NOFORCE) the proof is conditional, nothing is assumed, and the call stays --
// before the rule it was sound and assumed with the call out of line, which
// another TU's definition ignoring the precondition would have made wrong.
// Under -fno-contracts-prove-assume (NOASSUME) no assumption is kept, so
// nothing is forced either: conditional, and the real code is NOFORCE's.  A
// static callee's copy is this TU's: sound and assumed, the call left alone.
// An inline callee's assume precondition leaves nothing in Clang's real code,
// and observe leaves no premise.  p4405-premise-allowed.cpp has the label that
// settles the inline case.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-shadow-only.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

namespace sem_enforce_inline {
inline int h_inline (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_inline (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = sem_enforce_inline::h_inline (y);
  contract_assert (y > 0);  // force-remark {{proof shadow: proven, sound; assumed}} noforce-remark {{proof shadow: proven, conditional}} noassume-remark {{proof shadow: proven, conditional}}
  return r + (y > 0 ? 1 : 0);
}

namespace sem_enforce_static {
static int h_static (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_static (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = sem_enforce_static::h_static (y);
  contract_assert (y > 0);  // force-remark {{proof shadow: proven, sound; assumed}} noforce-remark {{proof shadow: proven, sound; assumed}} noassume-remark {{proof shadow: proven, sound}}
  return r + (y > 0 ? 1 : 0);
}

namespace sem_enforce_template {
template <class T>
int h_template (T x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_template (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = sem_enforce_template::h_template<int> (y);
  contract_assert (y > 0);  // force-remark {{proof shadow: proven, sound; assumed}} noforce-remark {{proof shadow: proven, conditional}} noassume-remark {{proof shadow: proven, conditional}}
  return r + (y > 0 ? 1 : 0);
}

namespace sem_assume_assume {
inline int h_assume (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_assume (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = sem_assume_assume::h_assume (y);
  contract_assert (y > 0);  // force-remark {{proof shadow: not proven}} force-warning {{contract assertion cannot be proven}} noforce-remark {{proof shadow: not proven}} noforce-warning {{contract assertion cannot be proven}} noassume-remark {{proof shadow: not proven}} noassume-warning {{contract assertion cannot be proven}}
  return r + (y > 0 ? 1 : 0);
}

namespace sem_observe_observe {
inline int h_observe (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_observe (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = sem_observe_observe::h_observe (y);
  contract_assert (y > 0);  // force-remark {{proof shadow: not proven}} force-warning {{contract assertion cannot be proven}} noforce-remark {{proof shadow: not proven}} noforce-warning {{contract assertion cannot be proven}} noassume-remark {{proof shadow: not proven}} noassume-warning {{contract assertion cannot be proven}}
  return r + (y > 0 ? 1 : 0);
}


// The forced calls are inlined; every other call stays out of line.
// FORCE-LABEL: define {{.*}} @_Z8g_inlineii(
// FORCE-NOT: call {{.*}} @{{.*}}h_inline{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z8g_inlineii(
// NOFORCE: call {{.*}} @{{.*}}h_inline{{.*}}(
// FORCE-LABEL: define {{.*}} @_Z8g_staticii(
// FORCE: call {{.*}} @{{.*}}h_static{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z8g_staticii(
// NOFORCE: call {{.*}} @{{.*}}h_static{{.*}}(
// FORCE-LABEL: define {{.*}} @_Z10g_templateii(
// FORCE-NOT: call {{.*}} @{{.*}}h_template{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z10g_templateii(
// NOFORCE: call {{.*}} @{{.*}}h_template{{.*}}(
// FORCE-LABEL: define {{.*}} @_Z8g_assumeii(
// FORCE: call {{.*}} @{{.*}}h_assume{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z8g_assumeii(
// NOFORCE: call {{.*}} @{{.*}}h_assume{{.*}}(
// FORCE-LABEL: define {{.*}} @_Z9g_observeii(
// FORCE: call {{.*}} @{{.*}}h_observe{{.*}}(
// NOFORCE-LABEL: define {{.*}} @_Z9g_observeii(
// NOFORCE: call {{.*}} @{{.*}}h_observe{{.*}}(
