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

// <contracts> declares the labels for every language mode it supports,
// whether or not the compiler implements P3400 labels, so they are exported
// unconditionally too.
export namespace std::contracts::labels {
  using std::contracts::labels::assertion_control_object;
  using std::contracts::labels::empty_label_t;
  using std::contracts::labels::empty_label;
  using std::contracts::labels::semantic_computation_label;
  using std::contracts::labels::allowed_semantics_label;
  using std::contracts::labels::local_violation_label;
  using std::contracts::labels::queryable_label;
  using std::contracts::labels::identification_label;
  using std::contracts::labels::compute_comment_label;
  using std::contracts::labels::compute_message_label;
  using std::contracts::labels::analysis_semantic_label;
  using std::contracts::labels::is_assertion_control_object;
  using std::contracts::labels::is_semantic_computation_label;
  using std::contracts::labels::is_allowed_semantics_label;
  using std::contracts::labels::is_local_violation_label;
  using std::contracts::labels::is_queryable_label;
  using std::contracts::labels::is_identification_label;
  using std::contracts::labels::is_compute_comment_label;
  using std::contracts::labels::is_compute_message_label;
  using std::contracts::labels::allowed_semantics_t;
  using std::contracts::labels::terminating;
  using std::contracts::labels::symbolic;
  using std::contracts::labels::always_enforce;
  using std::contracts::labels::always_quick_enforce;
  using std::contracts::labels::always_observe;
  using std::contracts::labels::always_ignore;
  using std::contracts::labels::never_enforce;
  using std::contracts::labels::never_quick_enforce;
  using std::contracts::labels::never_observe;
  using std::contracts::labels::never_ignore;
  using std::contracts::labels::assertion_group_label;
  using std::contracts::labels::operator""group;
  using std::contracts::labels::review_t;
  using std::contracts::labels::review;
  using std::contracts::labels::fixed_message_label_t;
  using std::contracts::labels::operator|;
} // namespace std::contracts::labels

// The short-name lookup <contracts> installs for the #include path, under the
// same condition as there.  It is a using-directive rather than a
// declaration, so a sweep over the header's public declarations does not list
// it.
#if defined(__cpp_contracts_labels)
export using contract_control namespace std::contracts::labels;
#endif // __cpp_contracts_labels
