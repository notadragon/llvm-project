// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontract-configuration-file=%S/c-p4405-premise-linkage.json -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang) in C: the callee's linkage and
// inlining.  A callee-side enforce precondition of a static, external, static
// inline, C99 inline (an inline definition, which Clang emits
// available_externally: the external definition the program calls is another
// TU's) or weak callee, always_inline, noinline or left to the inliner.  As in
// C++ (p4405-premise-linkage.cpp), the shadow relies on what it inlined from a
// replaceable callee only where the real function inlines it too (made so for a
// proof whose assumption it keeps), and a call it makes out of line to one
// holding such an evaluation makes it conditional.  Clang emits no copy at all
// of a noinline C99 inline definition, so the shadow calls a declaration and
// knows nothing of the definition the program calls: sound (gnu_gcc keeps the
// body and counts it: conditional).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-premise-linkage.c (gnu_gcc,
// whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

/* static, always */
__attribute__ ((always_inline)) static inline void h_static_always (int x) _Pre (x > 0) { sink (x); }

int
d_static_always (int x)
{
  h_static_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_static_always (int x, int y)
{
  h_static_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* static, never */
__attribute__ ((noinline)) static void h_static_never (int x) _Pre (x > 0) { sink (x); }

int
d_static_never (int x)
{
  h_static_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_static_never (int x, int y)
{
  h_static_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* static, auto */
static void h_static_auto (int x) _Pre (x > 0) { sink (x); }

int
d_static_auto (int x)
{
  h_static_auto (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_static_auto (int x, int y)
{
  h_static_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* extern, always */
__attribute__ ((always_inline)) void h_extern_always (int x) _Pre (x > 0) { sink (x); }

int
d_extern_always (int x)
{
  h_extern_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_extern_always (int x, int y)
{
  h_extern_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* extern, never */
__attribute__ ((noinline)) void h_extern_never (int x) _Pre (x > 0) { sink (x); }

int
d_extern_never (int x)
{
  h_extern_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_extern_never (int x, int y)
{
  h_extern_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* extern, auto */
void h_extern_auto (int x) _Pre (x > 0) { sink (x); }

int
d_extern_auto (int x)
{
  h_extern_auto (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_extern_auto (int x, int y)
{
  h_extern_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* static_inline, always */
__attribute__ ((always_inline)) static inline void h_static_inline_always (int x) _Pre (x > 0) { sink (x); }

int
d_static_inline_always (int x)
{
  h_static_inline_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_static_inline_always (int x, int y)
{
  h_static_inline_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* static_inline, never */
__attribute__ ((noinline)) static inline void h_static_inline_never (int x) _Pre (x > 0) { sink (x); }

int
d_static_inline_never (int x)
{
  h_static_inline_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_static_inline_never (int x, int y)
{
  h_static_inline_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* static_inline, auto */
static inline void h_static_inline_auto (int x) _Pre (x > 0) { sink (x); }

int
d_static_inline_auto (int x)
{
  h_static_inline_auto (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_static_inline_auto (int x, int y)
{
  h_static_inline_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* c99_inline, always */
__attribute__ ((always_inline)) inline void h_c99_inline_always (int x) _Pre (x > 0) { sink (x); }

int
d_c99_inline_always (int x)
{
  h_c99_inline_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_c99_inline_always (int x, int y)
{
  h_c99_inline_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* c99_inline, never */
__attribute__ ((noinline)) inline void h_c99_inline_never (int x) _Pre (x > 0) { sink (x); }

int
d_c99_inline_never (int x)
{
  h_c99_inline_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_c99_inline_never (int x, int y)
{
  h_c99_inline_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* c99_inline, auto */
inline void h_c99_inline_auto (int x) _Pre (x > 0) { sink (x); }

int
d_c99_inline_auto (int x)
{
  h_c99_inline_auto (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_c99_inline_auto (int x, int y)
{
  h_c99_inline_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* weak, always */
__attribute__ ((always_inline)) __attribute__ ((weak)) void h_weak_always (int x) _Pre (x > 0) { sink (x); }

int
d_weak_always (int x)
{
  h_weak_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_weak_always (int x, int y)
{
  h_weak_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

/* weak, never */
__attribute__ ((noinline)) __attribute__ ((weak)) void h_weak_never (int x) _Pre (x > 0) { sink (x); }

int
d_weak_never (int x)
{
  h_weak_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_weak_never (int x, int y)
{
  h_weak_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

/* weak, auto */
__attribute__ ((weak)) void h_weak_auto (int x) _Pre (x > 0) { sink (x); }

int
d_weak_auto (int x)
{
  h_weak_auto (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_weak_auto (int x, int y)
{
  h_weak_auto (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}
