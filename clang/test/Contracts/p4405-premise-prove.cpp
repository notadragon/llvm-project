// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-prove.json -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): a prove or diagnose evaluation
// as a premise.  It is guaranteed only where its proof is sound in this
// compilation, which is known only for this TU's own evaluations: a later
// evaluation in the same function relying on an unproven one is conditional, on
// a sound one sound, on a conditional one conditional.  A sound proof in an
// inlined callee is no premise for the caller's shadow -- the proof module
// keeps no assumption in a real function -- so after hp2 (assume, then prove)
// the caller's evaluation is unproven.  After hp (quick_enforce, then prove) it
// is proven all the same, by hp's own quick_enforce trap, which the optimizers
// see (gnu_gcc likewise since M5-GCC).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-prove.C (gnu_gcc,
// whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

int g_unproven (int x)
{
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-error {{contract assertion cannot be proven}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return sink (x);
}

int g_sound (int x)
{
  contract_assert (x > 0);  // quick_enforce
  contract_assert (x > 0);  // prove: expected-remark {{proof shadow: proven, sound; assumed}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return sink (x);
}

int g_conditional (int x)
{
  contract_assert (x > 0);  // observe
  contract_assert (x > 0);  // prove: expected-remark {{proof shadow: proven, conditional}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return sink (x);
}

[[gnu::always_inline]] inline int hp (int x)
{
  contract_assert (x > 0);  // quick_enforce
  contract_assert (x > 0);  // prove: expected-remark {{proof shadow: proven, sound; assumed}}
  return sink (x);
}

int g_callee (int x)
{
  hp (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

[[gnu::always_inline]] inline int hp2 (int x)
{
  contract_assert (x > 0);  // assume
  contract_assert (x > 0);  // prove: expected-remark {{proof shadow: proven, sound; assumed}}
  return sink (x);
}

int g_callee2 (int x)
{
  hp2 (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}
