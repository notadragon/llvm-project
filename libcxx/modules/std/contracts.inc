// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std::contracts {
  using std::contracts::assertion_kind;
  using std::contracts::contract_violation;
  using std::contracts::detection_mode;
  using std::contracts::evaluation_semantic;
  using std::contracts::evaluation_semantic_set;
  using std::contracts::invoke_default_contract_violation_handler;
  using std::contracts::is_checked;
  using std::contracts::is_continuing;
  using std::contracts::is_nonthrowing;
  using std::contracts::is_terminating;
  using std::contracts::is_unchecked;
  using std::contracts::violation_handled;
#if defined(__cpp_lib_contracts_api)
  // Also the nothrow_t overloads, whichever namespace declares them.
  using std::contracts::handle_enforced_contract_violation;
  using std::contracts::handle_observed_contract_violation;
  using std::contracts::handle_quick_enforced_contract_violation;
#endif // __cpp_lib_contracts_api
} // namespace std::contracts

// The P3400 labels, under the condition <contracts> declares them.
#if defined(__cpp_contracts_labels)
export namespace std::contracts::controls {
  using std::contracts::controls::assertion_control_object;
  using std::contracts::controls::empty_label_t;
  using std::contracts::controls::empty_label;
  using std::contracts::controls::defaulted;
  using std::contracts::controls::semantic_computation_label;
  using std::contracts::controls::allowed_semantics_label;
  using std::contracts::controls::local_violation_label;
  using std::contracts::controls::queryable_label;
  using std::contracts::controls::identification_label;
  using std::contracts::controls::compute_comment_label;
  using std::contracts::controls::compute_message_label;
  using std::contracts::controls::analysis_semantic_label;
  using std::contracts::controls::is_assertion_control_object;
  using std::contracts::controls::is_semantic_computation_label;
  using std::contracts::controls::is_allowed_semantics_label;
  using std::contracts::controls::is_local_violation_label;
  using std::contracts::controls::is_queryable_label;
  using std::contracts::controls::is_identification_label;
  using std::contracts::controls::is_compute_comment_label;
  using std::contracts::controls::is_compute_message_label;
  using std::contracts::controls::is_analysis_semantic_label;
  using std::contracts::controls::allowed_semantics_t;
  using std::contracts::controls::enforced;
  using std::contracts::controls::symbolic;
  using std::contracts::controls::always_enforce;
  using std::contracts::controls::always_quick_enforce;
  using std::contracts::controls::always_observe;
  using std::contracts::controls::always_ignore;
  using std::contracts::controls::never_enforce;
  using std::contracts::controls::never_quick_enforce;
  using std::contracts::controls::never_observe;
  using std::contracts::controls::never_ignore;
  using std::contracts::controls::assertion_group_label;
  using std::contracts::controls::operator""group;
  using std::contracts::controls::review_t;
  using std::contracts::controls::review;
  using std::contracts::controls::fixed_message_label_t;
  using std::contracts::controls::operator|;
#  if defined(__cpp_contracts_prove)
  // D4405's standard labels.
  using std::contracts::controls::prove_t;
  using std::contracts::controls::prove;
  using std::contracts::controls::diagnose_t;
  using std::contracts::controls::diagnose;
  using std::contracts::controls::always_prove_t;
  using std::contracts::controls::always_prove;
  using std::contracts::controls::always_diagnose_t;
  using std::contracts::controls::always_diagnose;
#  endif // __cpp_contracts_prove
} // namespace std::contracts::controls

// The short-name lookup <contracts> installs for the #include path.  It is a
// using-directive rather than a declaration, so a sweep over the header's
// public declarations does not list it.
export using contract_control namespace std::contracts::controls;
#endif // __cpp_contracts_labels

// P4451 without P3400: the defaulted and enforced labels alone, under the
// condition <contracts> declares them.
#if defined(__clang_contracts_p4451) && !defined(__cpp_contracts_labels)
export namespace std::contracts::controls {
  using std::contracts::controls::defaulted;
  using std::contracts::controls::enforced;
} // namespace std::contracts::controls
#endif // __clang_contracts_p4451 && !__cpp_contracts_labels
