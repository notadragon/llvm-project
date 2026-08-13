//===- contracts-abi.h - libcontracts ABI layout/constant defs ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/* This is the pure-C counterpart of libc++'s <__contracts/abi.h> (and
   libstdc++'s <bits/contracts_abi.h>).  The struct layouts and CXA_* values
   MUST match those headers, the compiler's contract codegen, and the
   cross-compiler contracts ABI specification.  */

#ifndef LIBCONTRACTS_CONTRACTS_ABI_H
#define LIBCONTRACTS_CONTRACTS_ABI_H 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Source location wire format.  Matches std::source_location::__impl.  */
typedef struct __cxa_source_location {
  const char *file_name;
  const char *function_name;
  unsigned line;
  unsigned column;
} __cxa_source_location;

/* Wire-format enum values (frozen; 0x00 is always "unspecified").  Plain C
   enums: the values are what matters; fields are stored as uint8_t.  */

enum /* __cxa_assertion_kind_t */
{
  CXA_AK_UNSPECIFIED = 0x00,
  CXA_AK_PRE = 0x01,
  CXA_AK_POST = 0x02,
  CXA_AK_ASSERT = 0x03,
  CXA_AK_MANUAL = 0x04,
  CXA_AK_CASSERT = 0x05,
  CXA_AK_POST_CAPTURE = 0x06,
  CXA_AK_IMPLICIT = 0x07 /* P3100 implicit contract assertion */
};

enum /* __cxa_evaluation_semantic_t */
{
  CXA_ES_UNSPECIFIED = 0x00,
  CXA_ES_IGNORE = 0x01,
  CXA_ES_OBSERVE = 0x02,
  CXA_ES_ENFORCE = 0x03,
  CXA_ES_QUICK_ENFORCE = 0x04,
  CXA_ES_NOEXCEPT_OBSERVE = 0x06,
  CXA_ES_NOEXCEPT_ENFORCE = 0x07
};

enum /* __cxa_detection_mode_t */
{
  CXA_DM_UNSPECIFIED = 0x00,
  CXA_DM_PREDICATE_FALSE = 0x01,
  CXA_DM_EVALUATION_EXCEPTION = 0x02
};

enum /* __cxa_field_id_t */
{
  CXA_FIELD_SOURCE_LOCATION = 0x01,
  CXA_FIELD_COMMENT = 0x02,
  CXA_FIELD_MESSAGE = 0x03,
  CXA_FIELD_LOCAL_HANDLER = 0x04,
  CXA_FIELD_QUERY_FUNCTION = 0x05,
  CXA_FIELD_LABEL_PTR = 0x06,
  CXA_FIELD_ASSERTION_KIND = 0x07,
  CXA_FIELD_EVALUATION_SEMANTIC = 0x08,
  CXA_FIELD_DETECTION_MODE = 0x09,
  CXA_FIELD_EXCEPTION_PTR = 0x0A,
  CXA_FIELD_REPORT = 0x0B, /* producer-populated lazy report */
  CXA_FIELD_EXTENDED = 0x40
};

enum /* __cxa_vendor_id_t */
{
  CXA_VENDOR_GENERIC = 0x0,
  CXA_VENDOR_GCC = 0x1,
  CXA_VENDOR_CLANG = 0x2,
  CXA_VENDOR_MSVC = 0x3
};

/* Descriptor data entry: byte offset (standard fields) or direct pointer
   (extended fields, ID >= 0x40).  */
union __cxa_descriptor_data_t {
  uintptr_t offset;
  const void *pointer;
};

/* Descriptor table.  Binary layout:
     byte 0:    [table_version : 4][vendor_id : 4]
     byte 1:    num_entries
     bytes 2..: field_ids[num_entries]
     <pad to alignof(union __cxa_descriptor_data_t)>
     data[num_entries]  */
typedef struct __cxa_descriptor_table_t {
  uint8_t header;
  uint8_t num_entries;
  uint8_t field_ids[];
} __cxa_descriptor_table_t;

/* One node in the linked list of violation data.  */
typedef struct __cxa_contract_data_block {
  const __cxa_descriptor_table_t *descriptor;
  const struct __cxa_contract_data_block *next;
  /* Field data follows, at byte offsets relative to this struct.  */
} __cxa_contract_data_block;

typedef int (*__cxa_local_handler_fn_t)(const void *label_ptr,
                                        const void *contract_violation_ptr);
typedef void *(*__cxa_query_fn_t)(const void *label_ptr, const void *key,
                                  size_t index);

/* Lazy report populator: report() calls populate(ctx) on demand and returns a
   producer-owned NUL-terminated string valid for the violation's lifetime.  */
typedef struct __cxa_contract_report_populator {
  const char *(*populate)(const void *ctx);
  const void *ctx;
} __cxa_contract_report_populator;

/* Chain-walking field lookup: first-found-wins over the chain.  */
const void *__cxa_find_field(const __cxa_contract_data_block *chain,
                             const uint8_t *ids, uint8_t num_ids);

/* Dispatch primitives, free of C++ exception handling.  The noexcept
   terminate-on-throw barrier lives in the C++ runtime, which wraps these.  */
void __contract_dispatch_core(const __cxa_contract_data_block *chain,
                              uint8_t semantic);
void __dispatch_with_override_core(void *data, uint8_t kind, uint8_t semantic,
                                   uint8_t mode);

/* Weak reference to the C++ runtime's noexcept terminate-on-throw wrapper
   around __contract_dispatch_core (defined in libstdc++/libc++'s
   contracts_abi.{cc,cpp}).  A pure-C entry point that dispatches a violation
   from inside a noexcept context -- notably __cxa_contract_violation_sanitizer
   below, called on a sanitizer runtime's noexcept report path -- must call this
   rather than the raw core so that a handler which exits via an exception under
   the noexcept evaluation semantics terminates the program (std::terminate)
   instead of escaping into frames that cannot unwind it.  Null when the C++
   runtime is not linked (freestanding); callers fall back to the raw core.  */
extern void
__contract_dispatch_core_noexcept(const __cxa_contract_data_block *chain,
                                  uint8_t semantic) __attribute__((weak));

/* Universal (non-noexcept) entry point.  */
void __cxa_contract_violation(void *data);

/* Specialized non-noexcept entry points (observe: return; enforce: noreturn).
   The _noexcept variants live in the C++ runtime.  */
void __cxa_contract_violation_pre_observe_pf(void *data);
void __cxa_contract_violation_pre_observe_ex(void *data);
void __cxa_contract_violation_post_observe_pf(void *data);
void __cxa_contract_violation_post_observe_ex(void *data);
void __cxa_contract_violation_assert_observe_pf(void *data);
void __cxa_contract_violation_assert_observe_ex(void *data);
void __cxa_contract_violation_post_capture_observe_pf(void *data);
void __cxa_contract_violation_post_capture_observe_ex(void *data);
void __cxa_contract_violation_implicit_observe_pf(void *data);
void __cxa_contract_violation_implicit_observe_ex(void *data);

void __cxa_contract_violation_pre_enforce_pf(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_pre_enforce_ex(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_post_enforce_pf(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_post_enforce_ex(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_assert_enforce_pf(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_assert_enforce_ex(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_post_capture_enforce_pf(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_post_capture_enforce_ex(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_implicit_enforce_pf(void *data)
    __attribute__((noreturn));
void __cxa_contract_violation_implicit_enforce_ex(void *data)
    __attribute__((noreturn));

/* C accessor functions (operate on a contract_violation object, whose layout
   is a single leading chain pointer).  */
const char *stdc_contract_violation_comment(const void *cv);
const char *stdc_contract_violation_file(const void *cv);
const char *stdc_contract_violation_function(const void *cv);
unsigned stdc_contract_violation_line(const void *cv);
unsigned stdc_contract_violation_column(const void *cv);
int stdc_contract_violation_kind(const void *cv);
int stdc_contract_violation_semantic(const void *cv);
int stdc_contract_violation_detection_mode(const void *cv);
int stdc_contract_violation_is_terminating(const void *cv);

/* P3290 C API.  */
void stdc_handle_enforced_contract_violation_explicit(const char *comment,
                                                      const char *file,
                                                      const char *func,
                                                      unsigned line)
    __attribute__((noreturn));
void stdc_handle_observed_contract_violation_explicit(const char *comment,
                                                      const char *file,
                                                      const char *func,
                                                      unsigned line);
void stdc_handle_quick_enforced_contract_violation_explicit(const char *comment,
                                                            const char *file,
                                                            const char *func,
                                                            unsigned line)
    __attribute__((noreturn));

/* P3100 Task 2.2 / CL2: sanitizer-report routing entry point.

   Called from inside a sanitizer runtime (e.g. compiler-rt asan), which
   detected an error but -- unlike in-code implicit checks -- has no
   compiler-generated data block.  This builds an implicit (CXA_AK_IMPLICIT)
   contract_violation from the discrete fields the runtime has and dispatches it
   to __handle_contract_violation.

   __semantic uses the ASan descriptor wire encoding, decoupled from the
   __cxa_evaluation_semantic_t values: 2 = enforce, anything else (1 = observe,
   0 = stock) => observe, the non-terminating default.  This entry point ALWAYS
   returns: the sanitizer runtime performs any termination itself (so it can run
   its own teardown / Die()), so the enforce-vs-observe continue/terminate
   decision stays in the caller.

   'report' is an optional lazy report populator (CXA_FIELD_REPORT): when
   non-NULL with a non-NULL populate function, a data block carrying it is
   chained onto the violation so contract_violation::report() invokes it on
   demand (the sanitizer prints nothing itself on the routed path).  Pass NULL
   for no report field.  */
void __cxa_contract_violation_sanitizer(
    const char *comment, const char *file, unsigned line,
    unsigned char semantic, const __cxa_contract_report_populator *report);

/* Compiler-emitted C contract-check helpers (D4299).  */
void __c_contract_check_enforce(const char *comment, const char *file,
                                const char *func, unsigned line,
                                unsigned char kind) __attribute__((noreturn));
void __c_contract_check_observe(const char *comment, const char *file,
                                const char *func, unsigned line,
                                unsigned char kind);
/* Build a block and dispatch through the noexcept terminate-on-throw barrier
   with an explicit core SEMANTIC (D4298 / P3100).  */
void __c_contract_check_noexcept(const char *comment, const char *file,
                                 const char *func, unsigned line,
                                 unsigned char kind, unsigned char semantic);

/* Handler resolution (see contracts ABI specification, section 8.4).
   Both are weak *references*: libcontracts never defines them.
   - __handle_contract_violation: the (optionally-replaced) global handler,
     defined as a strong C-linkage alias by the compiler for a user handler.
   - __contract_invoke_default_handler: the always-default handler, defined
     by the C++ runtime (libstdc++exp) when present.
   Both receive a pointer to a contract_violation object.  */
extern void __handle_contract_violation(const void *cv) __attribute__((weak));
extern void __contract_invoke_default_handler(const void *cv)
    __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif /* LIBCONTRACTS_CONTRACTS_ABI_H */
