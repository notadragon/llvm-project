//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// C contracts API (P4299)

#ifndef __CLANG_CONTRACTS_H
#define __CLANG_CONTRACTS_H

// The header's version, as every C23 standard header has one: the date of
// the implemented revision of P4299 until it is adopted.
#define __STDC_VERSION_CONTRACTS_H__ 202610L

#ifdef __cplusplus
extern "C" {
#endif

// Opaque violation type.  A const contract_violation_t* always points
// to a std::contracts::contract_violation object constructed by the
// runtime.  The struct is never defined in any C-visible header.
typedef struct contract_violation_t contract_violation_t;

// Accessor functions
const char *stdc_contract_violation_comment(const contract_violation_t *);
const char *stdc_contract_violation_file(const contract_violation_t *);
const char *stdc_contract_violation_function(const contract_violation_t *);
unsigned stdc_contract_violation_line(const contract_violation_t *);
unsigned stdc_contract_violation_column(const contract_violation_t *);
int stdc_contract_violation_kind(const contract_violation_t *);
int stdc_contract_violation_semantic(const contract_violation_t *);
int stdc_contract_violation_detection_mode(const contract_violation_t *);
int stdc_contract_violation_is_terminating(const contract_violation_t *);

// Assertion kind constants
#define STDC_CONTRACT_PRE 0x01
#define STDC_CONTRACT_POST 0x02
#define STDC_CONTRACT_ASSERT 0x03
#define STDC_CONTRACT_MANUAL 0x04
#define STDC_CONTRACT_CASSERT 0x05
#define STDC_CONTRACT_POST_CAPTURE 0x06
#define STDC_CONTRACT_IMPLICIT 0x07

// Evaluation semantic constants
#define STDC_CONTRACT_IGNORE 0x01
#define STDC_CONTRACT_OBSERVE 0x02
#define STDC_CONTRACT_ENFORCE 0x03
#define STDC_CONTRACT_QUICK_ENFORCE 0x04
/* Semantics P4299 does not name, which a handler can still receive: assume
   never reaches one, and the P4298 noexcept semantics are an extension in C
   (-fcontracts-p4298) and reach a handler shared with C++.  */
#define STDC_CONTRACT_ASSUME 0x05
#define STDC_CONTRACT_NOEXCEPT_OBSERVE 0x06
#define STDC_CONTRACT_NOEXCEPT_ENFORCE 0x07

// Detection mode constants
#define STDC_CONTRACT_UNSPECIFIED 0x00
#define STDC_CONTRACT_PREDICATE_FALSE 0x01
#define STDC_CONTRACT_EVALUATION_EXCEPTION 0x02

// P3290 C API -- explicit location variants
__attribute__((__noreturn__))
void stdc_handle_enforced_contract_violation_explicit(const char *__comment,
                                                      const char *__file,
                                                      const char *__func,
                                                      unsigned __line);

void stdc_handle_observed_contract_violation_explicit(const char *__comment,
                                                      const char *__file,
                                                      const char *__func,
                                                      unsigned __line);

__attribute__((__noreturn__))
void stdc_handle_quick_enforced_contract_violation_explicit(
    const char *__comment, const char *__file, const char *__func,
    unsigned __line);

// P3290 C API -- convenience macros
#define stdc_handle_enforced_contract_violation(comment)                       \
  stdc_handle_enforced_contract_violation_explicit((comment), __FILE__,        \
                                                   __func__, __LINE__)

#define stdc_handle_observed_contract_violation(comment)                       \
  stdc_handle_observed_contract_violation_explicit((comment), __FILE__,        \
                                                   __func__, __LINE__)

#define stdc_handle_quick_enforced_contract_violation(comment)                 \
  stdc_handle_quick_enforced_contract_violation_explicit((comment), __FILE__,  \
                                                         __func__, __LINE__)

// Contract check helpers (called by C compiler codegen)
__attribute__((__noreturn__))
void __c_contract_check_enforce(const char *__comment, const char *__file,
                                const char *__func, unsigned __line,
                                unsigned char __kind);

void __c_contract_check_observe(const char *__comment, const char *__file,
                                const char *__func, unsigned __line,
                                unsigned char __kind);

#ifdef __cplusplus
}
#endif

#endif // __CLANG_CONTRACTS_H
