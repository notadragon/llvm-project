//===- accessors.c - libcontracts contract_violation C accessors ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/* These implement the C accessor API for a contract_violation object.  Such
   an object has a single leading pointer to the head of the data-block chain
   (the same representation as std::contracts::contract_violation), so the C
   accessors read that chain directly via __cxa_find_field rather than calling
   into any C++ member function.  */

#include "contracts-abi.h"

/* A contract_violation object is { const __cxa_contract_data_block* chain; }.
 */
static const __cxa_contract_data_block *cv_chain(const void *cv) {
  return *(const __cxa_contract_data_block *const *)cv;
}

static const void *find1(const __cxa_contract_data_block *chain, uint8_t id) {
  uint8_t ids[1];
  ids[0] = id;
  return __cxa_find_field(chain, ids, 1);
}

static const __cxa_source_location *
cv_loc(const __cxa_contract_data_block *chain) {
  return (const __cxa_source_location *)find1(chain, CXA_FIELD_SOURCE_LOCATION);
}

static uint8_t cv_u8(const __cxa_contract_data_block *chain, uint8_t id,
                     uint8_t dflt) {
  const void *p = find1(chain, id);
  return p ? *(const uint8_t *)p : dflt;
}

const char *stdc_contract_violation_comment(const void *cv) {
  const void *p = find1(cv_chain(cv), CXA_FIELD_COMMENT);
  return p ? *(const char *const *)p : "";
}

const char *stdc_contract_violation_file(const void *cv) {
  const __cxa_source_location *loc = cv_loc(cv_chain(cv));
  return (loc && loc->file_name) ? loc->file_name : "";
}

const char *stdc_contract_violation_function(const void *cv) {
  const __cxa_source_location *loc = cv_loc(cv_chain(cv));
  return (loc && loc->function_name) ? loc->function_name : "";
}

unsigned stdc_contract_violation_line(const void *cv) {
  const __cxa_source_location *loc = cv_loc(cv_chain(cv));
  return loc ? loc->line : 0;
}

unsigned stdc_contract_violation_column(const void *cv) {
  const __cxa_source_location *loc = cv_loc(cv_chain(cv));
  return loc ? loc->column : 0;
}

int stdc_contract_violation_kind(const void *cv) {
  return (int)cv_u8(cv_chain(cv), CXA_FIELD_ASSERTION_KIND, CXA_AK_UNSPECIFIED);
}

int stdc_contract_violation_semantic(const void *cv) {
  return (int)cv_u8(cv_chain(cv), CXA_FIELD_EVALUATION_SEMANTIC,
                    CXA_ES_UNSPECIFIED);
}

int stdc_contract_violation_detection_mode(const void *cv) {
  return (int)cv_u8(cv_chain(cv), CXA_FIELD_DETECTION_MODE, CXA_DM_UNSPECIFIED);
}

int stdc_contract_violation_is_terminating(const void *cv) {
  uint8_t s =
      cv_u8(cv_chain(cv), CXA_FIELD_EVALUATION_SEMANTIC, CXA_ES_UNSPECIFIED);
  return (s == CXA_ES_ENFORCE || s == CXA_ES_QUICK_ENFORCE ||
          s == CXA_ES_NOEXCEPT_ENFORCE)
             ? 1
             : 0;
}
