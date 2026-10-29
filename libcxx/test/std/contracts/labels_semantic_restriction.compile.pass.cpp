//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: contracts

// ADDITIONAL_COMPILE_FLAGS: -fcontracts

// <contracts>
//
// P3400 [support.contract.labels.allowed], [support.contract.labels.sem] and
// the is_C variable templates of [support.contract.control.general]: what
// each standard semantic restriction label allows.  never_X allows every
// semantic of evaluation_semantic_set::all() but X, the extensions included
// (notadragon_wg21 DECISIONS-resolved.md E65).
//
// Mirror: libstdc++-v3/testsuite/18_support/contracts/labels_semantic_restriction.cc
// in the gnu_gcc fork.

#include <contracts>

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

static_assert(L::allowed_semantics_label<L::allowed_semantics_t<>>);
static_assert(L::allowed_semantics_t<>::allowed_semantics.empty());
static_assert(L::allowed_semantics_t<observe, assume>::allowed_semantics == evaluation_semantic_set{observe, assume});

template <auto X>
constexpr evaluation_semantic_set allowed = evaluation_semantic_set(decltype(X)::allowed_semantics);

// D4405: terminating also allows prove, symbolic diagnose and prove.
static_assert(allowed<L::terminating> == evaluation_semantic_set{enforce, quick_enforce, prove});
static_assert(allowed<L::symbolic> == evaluation_semantic_set{ignore, diagnose, prove});
static_assert(allowed<L::always_enforce> == evaluation_semantic_set{enforce});
static_assert(allowed<L::always_quick_enforce> == evaluation_semantic_set{quick_enforce});
static_assert(allowed<L::always_observe> == evaluation_semantic_set{observe});
static_assert(allowed<L::always_ignore> == evaluation_semantic_set{ignore});

static_assert(allowed<L::never_enforce> ==
              evaluation_semantic_set{
                  ignore, observe, quick_enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose});
static_assert(allowed<L::never_quick_enforce> ==
              evaluation_semantic_set{
                  ignore, observe, enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose});
static_assert(allowed<L::never_observe> ==
              evaluation_semantic_set{
                  ignore, enforce, quick_enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose});
static_assert(allowed<L::never_ignore> ==
              evaluation_semantic_set{
                  observe, enforce, quick_enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose});

static_assert(L::is_assertion_control_object<L::never_ignore>);
static_assert(L::is_allowed_semantics_label<L::terminating>);
static_assert(!L::is_allowed_semantics_label<L::review>);
static_assert(L::is_semantic_computation_label<L::review>);
static_assert(!L::is_semantic_computation_label<L::symbolic>);
static_assert(!L::is_local_violation_label<L::review>);
static_assert(!L::is_compute_comment_label<L::review>);
struct message_label {
  using assertion_control_object = message_label;
  constexpr const char* compute_message(const char*) const { return "m"; }
};
static_assert(L::is_compute_message_label<message_label{}>);
static_assert(!L::is_queryable_label<L::review>);
static_assert(L::is_identification_label<L::assertion_group_label<2>("g")>);
static_assert(!L::is_identification_label<L::review>);
static_assert(!L::is_assertion_control_object<0>);

// Combining keeps the intersection.
constexpr auto c = L::never_ignore | L::terminating;
static_assert(evaluation_semantic_set(c.allowed_semantics) == evaluation_semantic_set{enforce, quick_enforce, prove});
