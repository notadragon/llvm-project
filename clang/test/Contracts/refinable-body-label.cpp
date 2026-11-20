// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontracts-allow-assume %libcxx_flags -O2 -fcontract-evaluation-semantic=quick_enforce -S -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@_Z1gi(i32 noundef 1)' --implicit-check-not='@_Z1gi(i32 noundef 2)' --implicit-check-not='@_Z1gi(i32 noundef 3)' --implicit-check-not='@_Z1gi(i32 noundef 4)'
// REQUIRES: contracts-libcxx
//
// The refinable-body rule with P3400 labels (isContractEvaluationSettled):
// an evaluation is settled, and does not keep its function from being found
// nounwind from its body, when the label's allowed_semantics facet -- which
// every translation unit sees in the declaration -- leaves every copy the
// same choice as far as throwing goes: a subset of {quick_enforce,
// noexcept_enforce, assume}, or {enforce} alone, or in a noexcept function
// any subset of {enforce, quick_enforce, noexcept_enforce, assume}.  Every
// assertion here traps (quick_enforce) or is ignored in this translation
// unit, so no body can throw: the callers of an inline function whose
// evaluations are all settled drop their landing pads (no g (1) to g (4));
// those of one holding an evaluation that is not settled, or one without a
// label, keep them (g (11) to g (14)).  (Runnable/refinable-fork-label.cpp
// runs the sets across translation units.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-label.C (its
// nothrow half; LLVM takes no return value range from an inexact definition,
// settled or not, so the range half has nothing to show here)

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

inline constexpr allow<quick_enforce> l_quick{};
inline constexpr allow<quick_enforce, noexcept_enforce, assume> l_settled{};
inline constexpr allow<enforce, quick_enforce> l_mixed{};

int g (int);

// Settled.
[[gnu::noinline]] inline int a_quick (int x) { contract_assert<l_quick> (x > 0); return x; }
[[gnu::noinline]] inline int a_settled (int x) { contract_assert<l_settled> (x > 0); return x; }
[[gnu::noinline]] inline int a_pre (int x) pre<l_quick> (x > 0) { return x; }
[[gnu::noinline]] inline int a_mixed_nx (int x) noexcept { contract_assert<l_mixed> (x > 0); return x; }

// Not settled.
[[gnu::noinline]] inline int a_mixed (int x) { contract_assert<l_mixed> (x > 0); return x; }
[[gnu::noinline]] inline int a_none (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] inline int a_pre_none (int x) pre (x > 0) { return x; }
[[gnu::noinline]] inline int a_pre_some (int x) pre<l_quick> (x > 0) pre (x > 1) { return x; }

int use_quick (int y) { try { return a_quick (y); } catch (...) { return g (1); } }
int use_settled (int y) { try { return a_settled (y); } catch (...) { return g (2); } }
int use_pre (int y) { try { return a_pre (y); } catch (...) { return g (3); } }
int use_mixed_nx (int y) { try { return a_mixed_nx (y); } catch (...) { return g (4); } }
int use_mixed (int y) { try { return a_mixed (y); } catch (...) { return g (11); } }
int use_none (int y) { try { return a_none (y); } catch (...) { return g (12); } }
int use_pre_none (int y) { try { return a_pre_none (y); } catch (...) { return g (13); } }
int use_pre_some (int y) { try { return a_pre_some (y); } catch (...) { return g (14); } }

// CHECK-LABEL: define {{.*}}@_Z9use_mixedi(
// CHECK: invoke {{.*}}@_Z7a_mixedi(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 11)
// CHECK-LABEL: define {{.*}}@_Z8use_nonei(
// CHECK: invoke {{.*}}@_Z6a_nonei(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 12)
// CHECK-LABEL: define {{.*}}@_Z12use_pre_nonei(
// CHECK: invoke {{.*}}@_Z10a_pre_nonei(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 13)
// CHECK-LABEL: define {{.*}}@_Z12use_pre_somei(
// CHECK: invoke {{.*}}@_Z10a_pre_somei(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 14)
