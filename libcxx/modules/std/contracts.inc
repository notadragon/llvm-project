// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std::contracts {
  using std::contracts::contract_violation;
  using std::contracts::invoke_default_contract_violation_handler;
  using std::contracts::evaluation_semantic;
  using std::contracts::assertion_kind;
  using std::contracts::detection_mode;
} // namespace std::contracts

#if defined(__cpp_lib_contracts_labels)
export namespace std::contracts::labels {
  using std::contracts::labels::empty_label_t;
  using std::contracts::labels::empty_label;
  using std::contracts::labels::semantic_computation_label;
  using std::contracts::labels::allowed_semantics_label;
  using std::contracts::labels::local_violation_label;
  using std::contracts::labels::queryable_label;
  using std::contracts::labels::identification_label;
  using std::contracts::labels::compute_comment_label;
  using std::contracts::labels::compute_message_label;
  using std::contracts::labels::assertion_group_label;
  using std::contracts::labels::operator""group;
  using std::contracts::labels::review_t;
  using std::contracts::labels::review;
  using std::contracts::labels::fixed_message_label_t;
  using std::contracts::labels::operator|;
} // namespace std::contracts::labels

// The short-name lookup <contracts> installs for the #include path.  It is a
// using-directive rather than a declaration, so an export-completeness sweep
// over public non-uglified decls does not see it -- which is why the labels
// block above was missing too.
export using contract_control namespace std::contracts::labels;
#endif // __cpp_lib_contracts_labels
