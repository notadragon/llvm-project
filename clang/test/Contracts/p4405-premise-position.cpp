// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontract-configuration-file=%S/p4405-premise-position.json -Wno-contract-configuration -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): where the assertion is.  An
// inline callee's precondition evaluated callee-side or caller-side, its
// postcondition callee-side or caller-side, and a contract_assert in its body,
// all with enforce, the callee always_inline or noinline.  Clang implements no
// caller-side checking (the configuration's caller-side enforce is not emitted,
// and its warning is silenced), so a caller-side evaluation leaves nothing
// (GCC: decided here, in a TU-local wrapper). A noinline callee's postcondition
// is not seen by its caller: LLVM derives no return range from an inexact
// definition (GCC's IPA does, conditionally). Each consumer is a diagnose
// evaluation: d_* can be proven only from the callee's assertion, i_* from a
// dominating test, so only what makes the whole shadow conditional matters
// there.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-position.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

namespace calleeside_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_callee_always (int x)
{
  calleeside_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_pre_callee_always (int x, int y)
{
  calleeside_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace callerside_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_caller_always (int x)
{
  callerside_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_pre_caller_always (int x, int y)
{
  callerside_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace postcallee_always {
[[gnu::always_inline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_callee_always (int x)
{
  int r = postcallee_always::h (x);
  contract_assert (r > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_post_callee_always (int x, int y)
{
  postcallee_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace postcaller_always {
[[gnu::always_inline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_caller_always (int x)
{
  int r = postcaller_always::h (x);
  contract_assert (r > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_post_caller_always (int x, int y)
{
  postcaller_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace body_always {
[[gnu::always_inline]] inline void h (int x)  { contract_assert (x > 0); sink (x); }
}

int d_assert_always (int x)
{
  body_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_assert_always (int x, int y)
{
  body_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace calleeside_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_callee_never (int x)
{
  calleeside_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_pre_callee_never (int x, int y)
{
  calleeside_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace callerside_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_pre_caller_never (int x)
{
  callerside_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_pre_caller_never (int x, int y)
{
  callerside_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace postcallee_never {
[[gnu::noinline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_callee_never (int x)
{
  int r = postcallee_never::h (x);
  contract_assert (r > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_post_callee_never (int x, int y)
{
  postcallee_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace postcaller_never {
[[gnu::noinline]] inline int h (int x) post (r: r > 0) { return x; }
}

int d_post_caller_never (int x)
{
  int r = postcaller_never::h (x);
  contract_assert (r > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_post_caller_never (int x, int y)
{
  postcaller_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace body_never {
[[gnu::noinline]] inline void h (int x)  { contract_assert (x > 0); sink (x); }
}

int d_assert_never (int x)
{
  body_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_assert_never (int x, int y)
{
  body_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}
