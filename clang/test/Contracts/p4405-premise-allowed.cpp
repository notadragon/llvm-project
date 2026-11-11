// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume %libcxx_flags -O2 -fcontract-configuration-file=%S/p4405-premise-allowed.json -Wno-contract-configuration -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// REQUIRES: contracts-libcxx
//
// P4405 premises decided by this TU (T2-Clang): the allowed set.  When an
// assertion's label (P3400 allowed_semantics, which every TU sees in the
// declaration) admits only terminating semantics and assume, no TU can continue
// past a violation, so its evaluation is a guaranteed premise even where
// another TU's code may run: in a noinline replaceable callee the shadow calls,
// and in a callee inlined into the shadow but not into the real function
// (provoked as in p4405-premise-shadow-only.cpp), which the real function then
// keeps calling -- there only if the label also admits no semantic calling a
// handler, as settled_nothrow does: with settled, one TU's enforce handler may
// throw (or store) where another TU's quick_enforce copy traps, so the shadow
// may rely on the callee's code only if the real function inlines it too, and
// Clang makes it (M6-Clang; gnu_gcc's conditional). A label admitting observe,
// or none, settles nothing: the noinline case is conditional, and the provoked
// one is sound only because the real function is made to inline the callee.  A
// replaceable function that inlined an unsettled evaluation makes a shadow
// calling it out of line conditional (mid3); one that inlined a settled one
// does not (mid4). gnu_gcc's caller-side case (mid: a caller-side evaluation is
// never settled) leaves nothing in Clang, which implements no caller-side
// checking (its configuration warning silenced). Each consumer is a diagnose
// evaluation: d_* can be proven only from the callee's assertion, i_* from a
// dominating test, so only what makes the whole shadow conditional matters
// there.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-allowed.C
// (gnu_gcc, whose -fdump-tree-contract_shadow shows each result; here the
// -Rcontract-proof remark).

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

// Only terminating semantics and assume: every TU's evaluation of an
// assertion with one of these labels guarantees its predicate.
constexpr allow<enforce, quick_enforce, noexcept_enforce, assume> settled{};
constexpr allow<enforce> enforce_only{};
// A continuing semantic is allowed: another TU may evaluate it so.
constexpr allow<enforce, observe> open{};

// Settled for the refinable-body rule too: no copy's evaluation calls a
// handler, so none may throw where another's does not.
constexpr allow<quick_enforce, noexcept_enforce, assume> settled_nothrow{};

int sink (int);

// label settled, always
[[gnu::always_inline]] inline void h_settled_always (int x) pre<settled> (x > 0) { sink (x); }

int d_settled_always (int x)
{
  h_settled_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_settled_always (int x, int y)
{
  h_settled_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// label settled, never
[[gnu::noinline]] inline void h_settled_never (int x) pre<settled> (x > 0) { sink (x); }

int d_settled_never (int x)
{
  h_settled_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_settled_never (int x, int y)
{
  h_settled_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// label enforce_only, always
[[gnu::always_inline]] inline void h_enforce_only_always (int x) pre<enforce_only> (x > 0) { sink (x); }

int d_enforce_only_always (int x)
{
  h_enforce_only_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_enforce_only_always (int x, int y)
{
  h_enforce_only_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// label enforce_only, never
[[gnu::noinline]] inline void h_enforce_only_never (int x) pre<enforce_only> (x > 0) { sink (x); }

int d_enforce_only_never (int x)
{
  h_enforce_only_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_enforce_only_never (int x, int y)
{
  h_enforce_only_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// label open, always
[[gnu::always_inline]] inline void h_open_always (int x) pre<open> (x > 0) { sink (x); }

int d_open_always (int x)
{
  h_open_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_open_always (int x, int y)
{
  h_open_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// label open, never
[[gnu::noinline]] inline void h_open_never (int x) pre<open> (x > 0) { sink (x); }

int d_open_never (int x)
{
  h_open_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_open_never (int x, int y)
{
  h_open_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

// label none, always
[[gnu::always_inline]] inline void h_none_always (int x) pre (x > 0) { sink (x); }

int d_none_always (int x)
{
  h_none_always (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_none_always (int x, int y)
{
  h_none_always (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

// label none, never
[[gnu::noinline]] inline void h_none_never (int x) pre (x > 0) { sink (x); }

int d_none_never (int x)
{
  h_none_never (x);
  contract_assert (x > 0);  // expected-remark {{proof shadow: not proven}} expected-warning {{contract assertion cannot be proven}}
  return 0;
}

int i_none_never (int x, int y)
{
  h_none_never (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

inline int h_provoked_settled (int x) pre<settled> (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g_provoked_settled (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = h_provoked_settled (y);
  contract_assert (y > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return r + (y > 0 ? 1 : 0);
}

inline int h_provoked_open (int x) pre<open> (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g_provoked_open (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = h_provoked_open (y);
  contract_assert (y > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return r + (y > 0 ? 1 : 0);
}

inline int h_provoked_settled_nothrow (int x) pre<settled_nothrow> (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g_provoked_settled_nothrow (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = h_provoked_settled_nothrow (y);
  contract_assert (y > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return r + (y > 0 ? 1 : 0);
}

// Replaceable functions called out of line.
namespace cs {
[[gnu::noinline]] void hc (int x) pre<settled> (x > 0) { sink (x); }
}
[[gnu::noinline]] inline void mid (int y) { cs::hc (y); }
[[gnu::noinline]] inline void mid2 (int y) { h_settled_never (y); }
[[gnu::noinline]] inline void mid3 (int y) { h_open_always (y); }
[[gnu::noinline]] inline void mid4 (int y) { h_settled_always (y); }

int i_mid (int x, int y)
{
  mid (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_mid2 (int x, int y)
{
  mid2 (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}

int i_mid3 (int x, int y)
{
  mid3 (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, conditional}}
  return 0;
}

int i_mid4 (int x, int y)
{
  mid4 (y);
  if (x > 0)
    contract_assert (x > 0);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return 0;
}


// The provoked callers: a callee holding an evaluation that is not
// settled in the refinable-body sense is inlined (forced); the
// settled_nothrow one's call stays out of line.
// CHECK-LABEL: define {{.*}} @_Z18g_provoked_settledii(
// CHECK-NOT: call {{.*}} @_Z18h_provoked_settledi(
// CHECK: ret i32
// CHECK-LABEL: define {{.*}} @_Z15g_provoked_openii(
// CHECK-NOT: call {{.*}} @_Z15h_provoked_openi(
// CHECK: ret i32
// CHECK-LABEL: define {{.*}} @_Z26g_provoked_settled_nothrowii(
// CHECK: call {{.*}} @_Z26h_provoked_settled_nothrowi(
// CHECK: ret i32
