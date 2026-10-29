// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-p4298 -fcontracts-allow-assume -fcontract-configuration-file=%S/c-p4405-premise-semantic.json -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang) in C: the semantic of a C99
// inline callee's precondition, the callee always_inline or noinline.  enforce,
// quick_enforce (a direct trap) and noexcept_enforce (its entry point is
// noreturn in Clang; gnu_gcc's C noexcept entry point is not, so there it
// leaves none) leave a premise; assume lowers like ignore in Clang's real code
// (CLANG-111), and observe and ignore leave none.  Clang emits no copy of a
// noinline C99 inline definition, so a noinline callee leaves nothing whatever
// its semantic (gnu_gcc: conditional for the three that leave a premise).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-premise-semantic.c (gnu_gcc,
// whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

__attribute__ ((always_inline)) inline void h_enforce_always (int x) _Pre (x > 0) { sink (x); }

int
d_enforce_always (int x)
{
  h_enforce_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_enforce_always (int x, int y)
{
  h_enforce_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((noinline)) inline void h_enforce_never (int x) _Pre (x > 0) { sink (x); }

int
d_enforce_never (int x)
{
  h_enforce_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_enforce_never (int x, int y)
{
  h_enforce_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((always_inline)) inline void h_quick_enforce_always (int x) _Pre (x > 0) { sink (x); }

int
d_quick_enforce_always (int x)
{
  h_quick_enforce_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_quick_enforce_always (int x, int y)
{
  h_quick_enforce_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((noinline)) inline void h_quick_enforce_never (int x) _Pre (x > 0) { sink (x); }

int
d_quick_enforce_never (int x)
{
  h_quick_enforce_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_quick_enforce_never (int x, int y)
{
  h_quick_enforce_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((always_inline)) inline void h_assume_always (int x) _Pre (x > 0) { sink (x); }

int
d_assume_always (int x)
{
  h_assume_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_assume_always (int x, int y)
{
  h_assume_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((noinline)) inline void h_assume_never (int x) _Pre (x > 0) { sink (x); }

int
d_assume_never (int x)
{
  h_assume_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_assume_never (int x, int y)
{
  h_assume_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((always_inline)) inline void h_observe_always (int x) _Pre (x > 0) { sink (x); }

int
d_observe_always (int x)
{
  h_observe_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_observe_always (int x, int y)
{
  h_observe_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((noinline)) inline void h_observe_never (int x) _Pre (x > 0) { sink (x); }

int
d_observe_never (int x)
{
  h_observe_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_observe_never (int x, int y)
{
  h_observe_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((always_inline)) inline void h_noexcept_enforce_always (int x) _Pre (x > 0) { sink (x); }

int
d_noexcept_enforce_always (int x)
{
  h_noexcept_enforce_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int
i_noexcept_enforce_always (int x, int y)
{
  h_noexcept_enforce_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((noinline)) inline void h_noexcept_enforce_never (int x) _Pre (x > 0) { sink (x); }

int
d_noexcept_enforce_never (int x)
{
  h_noexcept_enforce_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_noexcept_enforce_never (int x, int y)
{
  h_noexcept_enforce_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((always_inline)) inline void h_ignore_always (int x) _Pre (x > 0) { sink (x); }

int
d_ignore_always (int x)
{
  h_ignore_always (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_ignore_always (int x, int y)
{
  h_ignore_always (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

__attribute__ ((noinline)) inline void h_ignore_never (int x) _Pre (x > 0) { sink (x); }

int
d_ignore_never (int x)
{
  h_ignore_never (x);
  _ContractAssert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int
i_ignore_never (int x, int y)
{
  h_ignore_never (y);
  if (x > 0)
    _ContractAssert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}
