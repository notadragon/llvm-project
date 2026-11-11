// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume %libcxx_flags -O2 -fcontract-configuration-file=%S/p4405-premise-late-summary-caller.json -Wno-contract-configuration -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// REQUIRES: contracts-libcxx
//
// P4405 premises decided by this TU (T2-Clang): as in
// p4405-premise-late-summary.cpp, but the replaceable function f's only
// contract evaluation that is not settled is a caller-side diagnose evaluation
// of cs::k's precondition, which the shadow proves soundly from h's enforced
// precondition: the shadow inlines the static h, whose copy is this TU's, while
// f calls it out of line (provoked as in p4405-premise-shadow-only.cpp, by an
// assume evaluation that a label settles). The assumption made of the proof
// tells the optimizers that the result is positive, but f is inline, so its
// definition is not exact (linkonce_odr) and LLVM infers nothing from it for a
// caller that does not inline it: use_f keeps its test of the result.  Clang
// needs nothing for this (gnu_gcc marks f a refinable body when it keeps the
// assumption).
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-late-summary-caller.C
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
constexpr allow<assume> assume_only{};

int bad ();
int sink (int);

namespace cs {
void k (int x) pre (x > 0);
}

static int h (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

[[gnu::noinline]] inline int f (int x, int y)
{
  contract_assert<assume_only> (x > 0);
  int r = 0;
  if (x > 0) [[unlikely]]
    r = h (y);
  cs::k (y);  // expected-remark {{proof shadow: proven, sound; assumed}}
  return r + y;
}

int use_f (int x, int y)
{
  int r = f (x, y);
  if (r <= 0)
    return bad ();
  return r;
}


// The real f calls h, and use_f still calls bad.
// CHECK-LABEL: define {{.*}} @_Z5use_fii(
// CHECK: call {{.*}} @_Z3badv(
// CHECK-LABEL: define linkonce_odr {{.*}} @_Z1fii(
// CHECK: call {{.*}} @_ZL1hi{{.*}}(
