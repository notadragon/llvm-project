//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: contracts

// ADDITIONAL_COMPILE_FLAGS: -fcontracts -fcontracts-p3400 -fcontracts-p4405

// <contracts>
//
// D3400R6 [support.contract.control.semset.general]: evaluation_semantic_set
// is a structural type, so a value of it can be a template argument; and
// [support.contract.control.combine]: a combined label is a structural type
// when both constituents are.  So every standard label, and every
// combination of them -- including those with an allowed_semantics
// constituent, which hold an evaluation_semantic_set -- can be the argument
// of the is_C variable templates and of any template<auto>.
//
// Mirror: libstdc++-v3/testsuite/18_support/contracts/structural_labels.cc
// in the gnu_gcc fork.

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using L::operator""group;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <auto V>
struct holder {
  static constexpr auto value = V;
};

template <typename T, typename U>
constexpr bool same = false;
template <typename T>
constexpr bool same<T, T> = true;

// An evaluation_semantic_set value as a template argument; equal sets are
// the same argument, different sets are not.
constexpr evaluation_semantic_set es{enforce};
constexpr evaluation_semantic_set eo{enforce, observe};
static_assert(holder<es>::value.contains(enforce));
static_assert(holder<eo>::value.size() == 2);
static_assert(same<holder<es>, holder<evaluation_semantic_set{enforce}>>);
static_assert(!same<holder<es>, holder<eo>>);
static_assert(!same<holder<evaluation_semantic_set::none()>, holder<evaluation_semantic_set::all()>>);
static_assert(holder<~es>::value == ~es);

// Each standard label as a template argument.
template <auto>
constexpr bool usable = true;
static_assert(usable<L::empty_label>);
static_assert(usable<L::terminating>);
static_assert(usable<L::symbolic>);
static_assert(usable<L::always_enforce>);
static_assert(usable<L::always_quick_enforce>);
static_assert(usable<L::always_observe>);
static_assert(usable<L::always_ignore>);
static_assert(usable<L::never_enforce>);
static_assert(usable<L::never_quick_enforce>);
static_assert(usable<L::never_observe>);
static_assert(usable<L::never_ignore>);
static_assert(usable<L::review>);
static_assert(usable<L::prove>);
static_assert(usable<L::diagnose>);
static_assert(usable<L::always_prove>);
static_assert(usable<L::always_diagnose>);
static_assert(usable<L::allowed_semantics_t<observe, ignore>{}>);
static_assert(usable<L::assertion_group_label("g")>);
static_assert(usable<"g"group>);

// A user label, structural, combined with terminating.
struct mine_t {
  using assertion_control_object = mine_t;
  int value = 3;
};
inline constexpr mine_t mine{};

// Every is_C variable template on combinations with an allowed_semantics
// constituent.
static_assert(L::is_assertion_control_object<L::terminating | L::symbolic>);
static_assert(L::is_allowed_semantics_label<L::terminating | L::symbolic>);
static_assert(!L::is_semantic_computation_label<L::terminating | L::symbolic>);
static_assert(!L::is_local_violation_label<L::terminating | L::symbolic>);
static_assert(!L::is_compute_comment_label<L::terminating | L::symbolic>);
static_assert(!L::is_compute_message_label<L::terminating | L::symbolic>);
static_assert(!L::is_queryable_label<L::terminating | L::symbolic>);
static_assert(!L::is_identification_label<L::terminating | L::symbolic>);
static_assert(!L::is_analysis_semantic_label<L::terminating | L::symbolic>);

static_assert(L::is_assertion_control_object<L::diagnose | L::always_enforce>);
static_assert(L::is_allowed_semantics_label<L::diagnose | L::always_enforce>);
static_assert(L::is_analysis_semantic_label<L::diagnose | L::always_enforce>);
static_assert(L::is_analysis_semantic_label<L::always_enforce | L::prove>);
static_assert(!L::is_semantic_computation_label<L::diagnose | L::always_enforce>);
static_assert(!L::is_identification_label<L::diagnose | L::always_enforce>);

// terminating with a group label (the runtime-cost label opt is a group
// named "opt").
static_assert(L::is_allowed_semantics_label<L::terminating | "opt"group>);
static_assert(L::is_identification_label<L::terminating | "opt"group>);
static_assert(!L::is_analysis_semantic_label<L::terminating | "opt"group>);
static_assert(L::is_identification_label<"a"group | "b"group | L::terminating>);

static_assert(L::is_assertion_control_object<mine | L::terminating>);
static_assert(L::is_allowed_semantics_label<mine | L::terminating>);
static_assert(L::is_allowed_semantics_label<L::terminating | mine>);
static_assert(!L::is_identification_label<mine | L::terminating>);

static_assert(L::is_semantic_computation_label<L::review | L::never_ignore>);
static_assert(L::is_allowed_semantics_label<L::review | L::never_ignore>);

// The combination's value survives being a template argument, and its
// identity is its value.
static_assert(holder<L::terminating | L::symbolic>::value.allowed_semantics == evaluation_semantic_set{prove});
static_assert(holder<L::diagnose | L::always_enforce>::value.allowed_semantics == evaluation_semantic_set{enforce});
static_assert(holder<L::diagnose | L::always_enforce>::value.analysis_semantic() == diagnose);
static_assert(holder<mine | L::terminating>::value.__lhs_.value == 3);
static_assert(*holder<L::terminating | "opt"group>::value.group_names.begin() == "opt");
static_assert(same<holder<L::terminating | L::symbolic>, holder<L::terminating | L::symbolic>>);
static_assert(!same<holder<"a"group | L::terminating>, holder<"b"group | L::terminating>>);
