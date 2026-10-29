// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontract-configuration-file=%S/p4405-premise-linkage.json -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): the callee's linkage and
// inlining.  A callee-side enforce precondition of a callee that is static,
// external, inline, a template specialization or weak, and always_inline,
// noinline or left to the inliner.  The shadow relies on what it inlined from a
// replaceable callee (an inexact definition: linkonce_odr, weak_odr, weak) only
// where the real function inlines it too: Clang makes those calls always_inline
// in the real function for a proof whose assumption it keeps, as here.  A
// non-replaceable callee's out-of-line copy is this TU's own code.  A call the
// shadow makes out of line to a replaceable callee holding such an evaluation
// makes it conditional.  Clang inlines an always_inline weak function (GCC
// refuses), and no other weak one.  Each consumer is a diagnose evaluation: d_*
// can be proven only from the callee's assertion, i_* from a dominating test,
// so only what makes the whole shadow conditional matters there.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-linkage.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

// static, always
[[gnu::always_inline]] static inline void h_static_always (int x) pre (x > 0) { sink (x); }

int d_static_always (int x)
{
  h_static_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_static_always (int x, int y)
{
  h_static_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// static, never
[[gnu::noinline]] static void h_static_never (int x) pre (x > 0) { sink (x); }

int d_static_never (int x)
{
  h_static_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_static_never (int x, int y)
{
  h_static_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// static, auto
static void h_static_auto (int x) pre (x > 0) { sink (x); }

int d_static_auto (int x)
{
  h_static_auto (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_static_auto (int x, int y)
{
  h_static_auto (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// extern, always
[[gnu::always_inline]] void h_extern_always (int x) pre (x > 0) { sink (x); }

int d_extern_always (int x)
{
  h_extern_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_extern_always (int x, int y)
{
  h_extern_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// extern, never
[[gnu::noinline]] void h_extern_never (int x) pre (x > 0) { sink (x); }

int d_extern_never (int x)
{
  h_extern_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_extern_never (int x, int y)
{
  h_extern_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// extern, auto
void h_extern_auto (int x) pre (x > 0) { sink (x); }

int d_extern_auto (int x)
{
  h_extern_auto (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_extern_auto (int x, int y)
{
  h_extern_auto (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// inline, always
[[gnu::always_inline]] inline void h_inline_always (int x) pre (x > 0) { sink (x); }

int d_inline_always (int x)
{
  h_inline_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_inline_always (int x, int y)
{
  h_inline_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// inline, never
[[gnu::noinline]] inline void h_inline_never (int x) pre (x > 0) { sink (x); }

int d_inline_never (int x)
{
  h_inline_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_inline_never (int x, int y)
{
  h_inline_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

// inline, auto
inline void h_inline_auto (int x) pre (x > 0) { sink (x); }

int d_inline_auto (int x)
{
  h_inline_auto (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_inline_auto (int x, int y)
{
  h_inline_auto (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// template, always
template <class T>
[[gnu::always_inline]] void h_template_always (T x) pre (x > 0) { sink (x); }

int d_template_always (int x)
{
  h_template_always<int> (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_template_always (int x, int y)
{
  h_template_always<int> (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// template, never
template <class T>
[[gnu::noinline]] void h_template_never (T x) pre (x > 0) { sink (x); }

int d_template_never (int x)
{
  h_template_never<int> (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_template_never (int x, int y)
{
  h_template_never<int> (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

// template, auto
template <class T>
void h_template_auto (T x) pre (x > 0) { sink (x); }

int d_template_auto (int x)
{
  h_template_auto<int> (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_template_auto (int x, int y)
{
  h_template_auto<int> (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// weak, always
[[gnu::weak]] [[gnu::always_inline]] void h_weak_always (int x) pre (x > 0) { sink (x); }

int d_weak_always (int x)
{
  h_weak_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_weak_always (int x, int y)
{
  h_weak_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// weak, never
[[gnu::weak]] [[gnu::noinline]] void h_weak_never (int x) pre (x > 0) { sink (x); }

int d_weak_never (int x)
{
  h_weak_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_weak_never (int x, int y)
{
  h_weak_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

// weak, auto
[[gnu::weak]] void h_weak_auto (int x) pre (x > 0) { sink (x); }

int d_weak_auto (int x)
{
  h_weak_auto (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_weak_auto (int x, int y)
{
  h_weak_auto (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}
