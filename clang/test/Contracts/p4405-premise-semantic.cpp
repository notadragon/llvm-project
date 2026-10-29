// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -O2 -fcontracts-p4298 -fcontracts-allow-assume -fcontract-configuration-file=%S/p4405-premise-semantic.json -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target
//
// P4405 premises decided by this TU (T2-Clang): the semantic of the callee's
// assertion in this TU. An inline callee's precondition with each of the nine
// semantics, the callee always_inline or noinline.  enforce, noexcept_enforce
// and quick_enforce leave code the shadow can take the predicate from --
// quick_enforce's trap is inline, visible to the optimizers (as in gnu_gcc
// since M5-GCC). assume lowers like ignore in Clang's real code (CLANG-111), so
// like ignore, observe, noexcept_observe, prove and diagnose it leaves none --
// the shadow never uses another function's proof.  A callee's prove or
// diagnose precondition is analyzed at each call (D4405), where it is not
// proven, and its unproven obligation is a premise for the consumer after it,
// which is then proven only conditionally.  Each consumer is a diagnose
// evaluation: d_* can be proven only from the callee's assertion, i_* from a
// dominating test, so only what makes the whole shadow conditional matters
// there.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-semantic.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

int sink (int);

namespace sem_ignore_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_ignore_always (int x)
{
  sem_ignore_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_ignore_always (int x, int y)
{
  sem_ignore_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_ignore_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_ignore_never (int x)
{
  sem_ignore_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_ignore_never (int x, int y)
{
  sem_ignore_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_observe_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_observe_always (int x)
{
  sem_observe_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_observe_always (int x, int y)
{
  sem_observe_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_observe_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_observe_never (int x)
{
  sem_observe_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_observe_never (int x, int y)
{
  sem_observe_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_enforce_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_enforce_always (int x)
{
  sem_enforce_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_enforce_always (int x, int y)
{
  sem_enforce_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_enforce_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_enforce_never (int x)
{
  sem_enforce_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_enforce_never (int x, int y)
{
  sem_enforce_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace sem_quick_enforce_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_quick_enforce_always (int x)
{
  sem_quick_enforce_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_quick_enforce_always (int x, int y)
{
  sem_quick_enforce_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_quick_enforce_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_quick_enforce_never (int x)
{
  sem_quick_enforce_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_quick_enforce_never (int x, int y)
{
  sem_quick_enforce_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace sem_assume_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_assume_always (int x)
{
  sem_assume_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_assume_always (int x, int y)
{
  sem_assume_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_assume_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_assume_never (int x)
{
  sem_assume_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_assume_never (int x, int y)
{
  sem_assume_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_noexcept_enforce_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_enforce_always (int x)
{
  sem_noexcept_enforce_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_noexcept_enforce_always (int x, int y)
{
  sem_noexcept_enforce_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_noexcept_enforce_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_enforce_never (int x)
{
  sem_noexcept_enforce_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_noexcept_enforce_never (int x, int y)
{
  sem_noexcept_enforce_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace sem_noexcept_observe_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_observe_always (int x)
{
  sem_noexcept_observe_always::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_noexcept_observe_always (int x, int y)
{
  sem_noexcept_observe_always::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_noexcept_observe_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }
}

int d_noexcept_observe_never (int x)
{
  sem_noexcept_observe_never::h (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_noexcept_observe_never (int x, int y)
{
  sem_noexcept_observe_never::h (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

namespace sem_prove_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }  // expected-note 2 {{the predicate of the contract assertion}}
}

int d_prove_always (int x)
{
  sem_prove_always::h (x);  // expected-remark {{proof shadow: not proven}} expected-error {{contract assertion cannot be proven}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

int i_prove_always (int x, int y)
{
  sem_prove_always::h (y);  // expected-remark {{proof shadow: not proven}} expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace sem_prove_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }  // expected-note 2 {{the predicate of the contract assertion}}
}

int d_prove_never (int x)
{
  sem_prove_never::h (x);  // expected-remark {{proof shadow: not proven}} expected-error {{contract assertion cannot be proven}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

int i_prove_never (int x, int y)
{
  sem_prove_never::h (y);  // expected-remark {{proof shadow: not proven}} expected-error {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace sem_diagnose_always {
[[gnu::always_inline]] inline void h (int x) pre (x > 0) { sink (x); }  // expected-note 2 {{the predicate of the contract assertion}}
}

int d_diagnose_always (int x)
{
  sem_diagnose_always::h (x);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

int i_diagnose_always (int x, int y)
{
  sem_diagnose_always::h (y);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

namespace sem_diagnose_never {
[[gnu::noinline]] inline void h (int x) pre (x > 0) { sink (x); }  // expected-note 2 {{the predicate of the contract assertion}}
}

int d_diagnose_never (int x)
{
  sem_diagnose_never::h (x);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

int i_diagnose_never (int x, int y)
{
  sem_diagnose_never::h (y);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}
