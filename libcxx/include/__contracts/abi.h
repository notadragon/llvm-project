//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_CONTRACTS_ABI_H
#define _LIBCPP_CONTRACTS_ABI_H

// Contracts ABI wire-format types.
// This header defines the binary interface shared between the compiler-
// generated contract-check code and the ABI runtime (libstdc++/libc++).
// All types and enumerators must remain binary-compatible with the GCC
// implementation in bits/contracts_abi.h.

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#include <cstdint>
#include <cstddef>

namespace __cxxabiv1 {

// Source location wire format.
// Layout matches std::source_location::__impl in both libstdc++ and libc++.
struct __cxa_source_location
{
  const char* file_name;
  const char* function_name;
  unsigned line;
  unsigned column;
};

// ABI wire-format enums.
// Decoupled from std::contracts enum representations.
// 0x00 is always "unspecified" (matches default when field is absent).

enum __cxa_assertion_kind_t : __UINT8_TYPE__
{
  CXA_AK_UNSPECIFIED    = 0x00,
  CXA_AK_PRE            = 0x01,
  CXA_AK_POST           = 0x02,
  CXA_AK_ASSERT         = 0x03,
  CXA_AK_MANUAL         = 0x04,
  CXA_AK_CASSERT        = 0x05,
  CXA_AK_POST_CAPTURE   = 0x06,
  CXA_AK_IMPLICIT       = 0x07,  // P3100 implicit contract assertion
};

enum __cxa_evaluation_semantic_t : __UINT8_TYPE__
{
  CXA_ES_UNSPECIFIED    = 0x00,
  CXA_ES_IGNORE         = 0x01,
  CXA_ES_OBSERVE        = 0x02,
  CXA_ES_ENFORCE        = 0x03,
  CXA_ES_QUICK_ENFORCE  = 0x04,
  CXA_ES_NOEXCEPT_OBSERVE = 0x06,
  CXA_ES_NOEXCEPT_ENFORCE = 0x07,
};

enum __cxa_detection_mode_t : __UINT8_TYPE__
{
  CXA_DM_UNSPECIFIED          = 0x00,
  CXA_DM_PREDICATE_FALSE      = 0x01,
  CXA_DM_EVALUATION_EXCEPTION = 0x02,
};

// Field IDs.
// Each ID identifies both a logical field and its wire-format encoding.
// Multiple IDs can represent the same logical field with different encodings;
// accessors search for any matching ID and take the first found.

enum __cxa_field_id_t : __UINT8_TYPE__
{
  // Standard fields (< 0x40): use byte offset into containing data block.
  CXA_FIELD_SOURCE_LOCATION     = 0x01,
  CXA_FIELD_COMMENT             = 0x02,
  CXA_FIELD_MESSAGE             = 0x03,
  CXA_FIELD_LOCAL_HANDLER       = 0x04,
  CXA_FIELD_QUERY_FUNCTION      = 0x05,
  CXA_FIELD_LABEL_PTR           = 0x06,
  CXA_FIELD_ASSERTION_KIND      = 0x07,
  CXA_FIELD_EVALUATION_SEMANTIC = 0x08,
  CXA_FIELD_DETECTION_MODE      = 0x09,
  CXA_FIELD_EXCEPTION_PTR       = 0x0A,
  CXA_FIELD_REPORT              = 0x0B,  // producer-populated lazy report
  // 0x0C - 0x3F: reserved for future standard fields.

  // Extended fields (>= 0x40): use direct pointer, not offset.
  CXA_FIELD_EXTENDED            = 0x40,
  // 0x40 - 0x7F: vendor-specific extensions.
  // 0x80 - 0xFF: reserved for future use.
};

// Vendor IDs (4-bit, packed into descriptor table header).
enum __cxa_vendor_id_t : __UINT8_TYPE__
{
  CXA_VENDOR_GENERIC = 0x0,
  CXA_VENDOR_GCC     = 0x1,
  CXA_VENDOR_CLANG   = 0x2,
  CXA_VENDOR_MSVC    = 0x3,
};

// Descriptor data entry.
// For standard fields (ID < 0x40), 'offset' is a byte offset into the
// containing data block.  For extended fields (ID >= 0x40), 'pointer'
// is a direct pointer to the data.
union __cxa_descriptor_data_t
{
  __UINTPTR_TYPE__ offset;
  const void*      pointer;
};

// Descriptor table.
// Describes which fields are present in a data block and where to find them.
//
// Binary layout:
//   byte 0:     [table_version : 4 bits][vendor_id : 4 bits]
//   byte 1:     num_entries
//   bytes 2..:  field_ids[num_entries]
//   <padding to alignof(__cxa_descriptor_data_t)>
//   data[num_entries] (__cxa_descriptor_data_t)
//
// The field_ids and data arrays are parallel: field_ids[i] describes
// the kind of data, data[i] tells where to find it.
struct __cxa_descriptor_table_t
{
  __UINT8_TYPE__ header;
  __UINT8_TYPE__ num_entries;
  __UINT8_TYPE__ field_ids[];

  __UINT8_TYPE__
  table_version () const
  { return header >> 4; }

  __UINT8_TYPE__
  vendor_id () const
  { return header & 0x0F; }

  const __cxa_descriptor_data_t*
  data () const
  {
    auto __p = reinterpret_cast<__UINTPTR_TYPE__>(field_ids + num_entries);
    __p = (__p + alignof(__cxa_descriptor_data_t) - 1)
          & ~(alignof(__cxa_descriptor_data_t) - 1);
    return reinterpret_cast<const __cxa_descriptor_data_t*>(__p);
  }
};

// Data block.
// One node in a linked list of violation data.  Each block has its own
// descriptor table and field data.  The runtime walks head-to-tail,
// taking the first occurrence of each field (first-found-wins).
struct __cxa_contract_data_block
{
  const __cxa_descriptor_table_t*  descriptor;
  const __cxa_contract_data_block* next;
  // Field data follows (layout described by descriptor,
  // byte offsets relative to the start of this struct).
};

// Local handler function pointer type.
// The second argument is a pointer to the contract_violation object
// (passed as void* to avoid circular dependency on std::contracts).
using __cxa_local_handler_fn_t
    = int (*)(const void* /*label_ptr*/,
              const void* /*contract_violation_ptr*/);

// Query function pointer type.
using __cxa_query_fn_t
    = void* (*)(const void* /*label_ptr*/,
                const void* /*key*/,
                std::size_t /*index*/);

// Lazy report populator: report() calls populate(ctx) on demand and returns a
// producer-owned NUL-terminated string valid for the violation's lifetime.
struct __cxa_contract_report_populator
{
  const char* (*populate) (const void* __ctx);
  const void* ctx;
};

// Chain-walking field lookup.
// Walks the chain head-to-tail, searching each block's descriptor for
// any field ID in the 'ids' array.  Returns a pointer to the field data
// within the block where the first match was found, or nullptr.
// C linkage: this is an ABI entry point provided by libcontracts (pure C).
extern "C" const void*
__cxa_find_field (const __cxa_contract_data_block* __chain,
                  const __UINT8_TYPE__* __ids,
                  __UINT8_TYPE__ __num_ids);

// Typed convenience: find a scalar value field.
template<typename _Tp>
  _Tp
  __cxa_find_field_value (const __cxa_contract_data_block* __chain,
                          __UINT8_TYPE__ __field_id,
                          _Tp __default_value)
  {
    __UINT8_TYPE__ __ids[] = { __field_id };
    auto __p = __cxa_find_field(__chain, __ids, 1);
    if (__p)
      return *static_cast<const _Tp*>(__p);
    return __default_value;
  }

// Typed convenience: find a pointer to structured field data.
template<typename _Tp>
  const _Tp*
  __cxa_find_field_ptr (const __cxa_contract_data_block* __chain,
                        __UINT8_TYPE__ __field_id)
  {
    __UINT8_TYPE__ __ids[] = { __field_id };
    return static_cast<const _Tp*>(
        __cxa_find_field(__chain, __ids, 1));
  }

} // namespace __cxxabiv1

// Entry points.
// The non-noexcept entry points and the dispatch primitives are provided by
// the pure-C libcontracts; the _noexcept variants are provided by libc++
// (they need a C++ exception-handling barrier).
// All take a single void* pointing to the head of a data block chain.
extern "C" {

// Pure-C dispatch primitives, provided by libcontracts.  The C++ _noexcept
// entry points wrap these in a terminate-on-throw barrier.
void __contract_dispatch_core (const __cxxabiv1::__cxa_contract_data_block* __chain,
                               __UINT8_TYPE__ __semantic);
void __dispatch_with_override_core (void* __data, __UINT8_TYPE__ __kind,
                                    __UINT8_TYPE__ __semantic,
                                    __UINT8_TYPE__ __mode);

// Compiler-emitted C contract-check helpers, provided by libcontracts.  The
// pure-virtual termini below reuse these to build and dispatch a generic
// implicit violation.  _enforce is [[noreturn]] (the enforcing core aborts);
// _noexcept dispatches through the terminate-on-throw barrier with an explicit
// core semantic.
[[noreturn]] void __c_contract_check_enforce (
    const char* __comment, const char* __file, const char* __func,
    unsigned __line, unsigned char __kind);
void __c_contract_check_observe (
    const char* __comment, const char* __file, const char* __func,
    unsigned __line, unsigned char __kind);
void __c_contract_check_noexcept (
    const char* __comment, const char* __file, const char* __func,
    unsigned __line, unsigned char __kind, unsigned char __semantic);

// Noexcept terminate-on-throw wrapper around __contract_dispatch_core, invoked
// with an explicit core semantic.  Provided by libc++ (contracts_abi.cpp) and
// referenced weakly by the pure-C libcontracts sanitizer-report routing entry
// point so a throwing handler under the noexcept semantics terminates.
void __contract_dispatch_core_noexcept (
    const __cxxabiv1::__cxa_contract_data_block* __chain,
    __UINT8_TYPE__ __semantic) noexcept;

void __cxa_contract_violation (void* __data);
void __cxa_contract_violation_noexcept (void* __data) noexcept;

// Specialized: pre + observe
void __cxa_contract_violation_pre_observe_pf (void* __data);
void __cxa_contract_violation_pre_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_pre_observe_ex (void* __data);
void __cxa_contract_violation_pre_observe_ex_noexcept (void* __data) noexcept;

// Specialized: post + observe
void __cxa_contract_violation_post_observe_pf (void* __data);
void __cxa_contract_violation_post_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_post_observe_ex (void* __data);
void __cxa_contract_violation_post_observe_ex_noexcept (void* __data) noexcept;

// Specialized: assert + observe
void __cxa_contract_violation_assert_observe_pf (void* __data);
void __cxa_contract_violation_assert_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_assert_observe_ex (void* __data);
void __cxa_contract_violation_assert_observe_ex_noexcept (void* __data) noexcept;

// Specialized: pre + enforce
[[noreturn]] void __cxa_contract_violation_pre_enforce_pf (void* __data);
[[noreturn]] void __cxa_contract_violation_pre_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_pre_enforce_ex (void* __data);
[[noreturn]] void __cxa_contract_violation_pre_enforce_ex_noexcept (void* __data) noexcept;

// Specialized: post + enforce
[[noreturn]] void __cxa_contract_violation_post_enforce_pf (void* __data);
[[noreturn]] void __cxa_contract_violation_post_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_post_enforce_ex (void* __data);
[[noreturn]] void __cxa_contract_violation_post_enforce_ex_noexcept (void* __data) noexcept;

// Specialized: assert + enforce
[[noreturn]] void __cxa_contract_violation_assert_enforce_pf (void* __data);
[[noreturn]] void __cxa_contract_violation_assert_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_assert_enforce_ex (void* __data);
[[noreturn]] void __cxa_contract_violation_assert_enforce_ex_noexcept (void* __data) noexcept;

// Specialized: post_capture + observe
void __cxa_contract_violation_post_capture_observe_pf (void* __data);
void __cxa_contract_violation_post_capture_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_post_capture_observe_ex (void* __data);
void __cxa_contract_violation_post_capture_observe_ex_noexcept (void* __data) noexcept;

// Specialized: post_capture + enforce
[[noreturn]] void __cxa_contract_violation_post_capture_enforce_pf (void* __data);
[[noreturn]] void __cxa_contract_violation_post_capture_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_post_capture_enforce_ex (void* __data);
[[noreturn]] void __cxa_contract_violation_post_capture_enforce_ex_noexcept (void* __data) noexcept;

// Specialized: implicit + observe (P3100)
void __cxa_contract_violation_implicit_observe_pf (void* __data);
void __cxa_contract_violation_implicit_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_implicit_observe_ex (void* __data);
void __cxa_contract_violation_implicit_observe_ex_noexcept (void* __data) noexcept;

// Specialized: implicit + enforce (P3100)
[[noreturn]] void __cxa_contract_violation_implicit_enforce_pf (void* __data);
[[noreturn]] void __cxa_contract_violation_implicit_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_implicit_enforce_ex (void* __data);
[[noreturn]] void __cxa_contract_violation_implicit_enforce_ex_noexcept (void* __data) noexcept;

// Specialized: noexcept_observe (D4298).  Defined in libc++ (contracts_abi.cpp);
// each wraps a dispatch primitive in a terminate-on-throw barrier.
void __cxa_contract_violation_pre_noexcept_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_pre_noexcept_observe_ex_noexcept (void* __data) noexcept;
void __cxa_contract_violation_post_noexcept_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_post_noexcept_observe_ex_noexcept (void* __data) noexcept;
void __cxa_contract_violation_assert_noexcept_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_assert_noexcept_observe_ex_noexcept (void* __data) noexcept;
void __cxa_contract_violation_post_capture_noexcept_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_post_capture_noexcept_observe_ex_noexcept (void* __data) noexcept;
void __cxa_contract_violation_implicit_noexcept_observe_pf_noexcept (void* __data) noexcept;
void __cxa_contract_violation_implicit_noexcept_observe_ex_noexcept (void* __data) noexcept;

// Specialized: noexcept_enforce ([[noreturn]], D4298).
[[noreturn]] void __cxa_contract_violation_pre_noexcept_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_pre_noexcept_enforce_ex_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_post_noexcept_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_post_noexcept_enforce_ex_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_assert_noexcept_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_assert_noexcept_enforce_ex_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_post_capture_noexcept_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_post_capture_noexcept_enforce_ex_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_implicit_noexcept_enforce_pf_noexcept (void* __data) noexcept;
[[noreturn]] void __cxa_contract_violation_implicit_noexcept_enforce_ex_noexcept (void* __data) noexcept;

// P3100: pure-virtual-call terminus variants (ub:class.abstract.pure.virtual).
// The compiler points a pure virtual's vtable slot at one of these instead of
// the legacy __cxa_pure_virtual when the class's implicit contract
// configuration selects a checking semantic.  See contracts_abi.cpp for the
// selection and termination rationale.
[[noreturn]] void __cxa_pure_virtual_quick (void) noexcept;
[[noreturn]] void __cxa_pure_virtual_enforce (void);
[[noreturn]] void __cxa_pure_virtual_noexcept_enforce (void) noexcept;
[[noreturn]] void __cxa_pure_virtual_observe (void);
[[noreturn]] void __cxa_pure_virtual_noexcept_observe (void) noexcept;

} // extern "C"

#endif // _LIBCPP_CONTRACTS_ABI_H
