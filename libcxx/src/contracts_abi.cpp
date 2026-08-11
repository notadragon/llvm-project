//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Contracts ABI noexcept entry points.
//
// The contracts ABI dispatch core, chain walking, and the non-noexcept entry
// points live in the pure-C libcontracts.  What remains here are the C++
// _noexcept entry-point variants: each wraps a libcontracts dispatch
// primitive in a noexcept terminate-on-throw barrier (a handler that exits
// via an exception at a noexcept site terminates the program via
// std::terminate(), with the exception still active).  This is the one part
// of the contracts runtime that requires C++ exception handling, so it
// cannot live in the pure-C core.

#include "../include/contracts"
#include <__contracts/abi.h>
#include <exception>

using namespace __cxxabiv1;

// D4301: contract_violation::report().  Finds the CXA_FIELD_REPORT data-block
// field and, if a populator is present, invokes it on demand to obtain a
// producer-owned diagnostic string.  Returns nullptr when no report field is
// present (the common case for ordinary contract violations), so the populator
// is never invoked unless a caller actually asks for the report.  noexcept:
// on-demand rendering may allocate (and a populator could throw), so the
// populator call is guarded; a throwing populator yields the static-storage
// literal "Error generating report" rather than escaping the noexcept
// accessor.
const char*
std::contracts::contract_violation::report() const noexcept {
  auto __p = __cxa_find_field_ptr<__cxa_contract_report_populator>(
      __chain_, CXA_FIELD_REPORT);
  if (!__p || !__p->populate)
    return nullptr;
#if _LIBCPP_HAS_EXCEPTIONS
  try {
    return __p->populate(__p->ctx);
  } catch (...) {
    return "Error generating report";
  }
#else
  return __p->populate(__p->ctx);
#endif
}

namespace {

// Run the override dispatch under the noexcept barrier.
void __nx_override(void* __data, __UINT8_TYPE__ __kind,
                   __UINT8_TYPE__ __semantic, __UINT8_TYPE__ __mode) {
#if _LIBCPP_HAS_EXCEPTIONS
  try {
    __dispatch_with_override_core(__data, __kind, __semantic, __mode);
  } catch (...) {
    std::terminate();
  }
#else
  __dispatch_with_override_core(__data, __kind, __semantic, __mode);
#endif
}

} // namespace

// Universal noexcept entry point.
extern "C" _LIBCPP_EXPORTED_FROM_ABI void
__cxa_contract_violation_noexcept (void* __data) noexcept {
  auto* __chain = static_cast<const __cxa_contract_data_block*>(__data);
  __UINT8_TYPE__ __sem = __cxa_find_field_value<__UINT8_TYPE__>(
      __chain, CXA_FIELD_EVALUATION_SEMANTIC, CXA_ES_UNSPECIFIED);
#if _LIBCPP_HAS_EXCEPTIONS
  try { __contract_dispatch_core(__chain, __sem); }
  catch (...) { std::terminate(); }
#else
  __contract_dispatch_core(__chain, __sem);
#endif
}

// Noexcept terminate-on-throw wrapper around __contract_dispatch_core, invoked
// with an explicit core semantic.  The pure-C libcontracts declares this as a
// weak reference (contracts-abi.h) and its sanitizer-report routing entry point
// calls it in place of the raw core: that entry runs on the sanitizer runtime's
// noexcept report path under the D4298 noexcept evaluation semantics, so a
// handler that exits via an exception must terminate the program here rather
// than escape into frames that cannot unwind it.
extern "C" _LIBCPP_EXPORTED_FROM_ABI void
__contract_dispatch_core_noexcept (const __cxa_contract_data_block* __chain,
                                   __UINT8_TYPE__ __semantic) noexcept {
#if _LIBCPP_HAS_EXCEPTIONS
  try { __contract_dispatch_core(__chain, __semantic); }
  catch (...) { std::terminate(); }
#else
  __contract_dispatch_core(__chain, __semantic);
#endif
}

// Specialized noexcept entry points: observe.
#define CXA_OBSERVE_NX(kind_name, kind_val, mode_name, mode_val)             \
  extern "C" _LIBCPP_EXPORTED_FROM_ABI void                                  \
  __cxa_contract_violation_##kind_name##_observe_##mode_name##_noexcept      \
      (void* __data) noexcept {                                             \
    __nx_override(__data, kind_val, CXA_ES_OBSERVE, mode_val);              \
  }

CXA_OBSERVE_NX(pre,          CXA_AK_PRE,          pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(pre,          CXA_AK_PRE,          ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(post,         CXA_AK_POST,         pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(post,         CXA_AK_POST,         ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(assert,       CXA_AK_ASSERT,       pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(assert,       CXA_AK_ASSERT,       ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(post_capture, CXA_AK_POST_CAPTURE, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(implicit,     CXA_AK_IMPLICIT,     ex, CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_OBSERVE_NX

// Specialized noexcept entry points: enforce ([[noreturn]]).
#define CXA_ENFORCE_NX(kind_name, kind_val, mode_name, mode_val)             \
  extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void                     \
  __cxa_contract_violation_##kind_name##_enforce_##mode_name##_noexcept      \
      (void* __data) noexcept {                                             \
    __nx_override(__data, kind_val, CXA_ES_ENFORCE, mode_val);              \
    __builtin_unreachable();                                                \
  }

CXA_ENFORCE_NX(pre,          CXA_AK_PRE,          pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(pre,          CXA_AK_PRE,          ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(post,         CXA_AK_POST,         pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(post,         CXA_AK_POST,         ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(assert,       CXA_AK_ASSERT,       pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(assert,       CXA_AK_ASSERT,       ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(post_capture, CXA_AK_POST_CAPTURE, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(implicit,     CXA_AK_IMPLICIT,     ex, CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_ENFORCE_NX

// Specialized entry points: noexcept_observe (D4298).
#define CXA_NOEXCEPT_OBSERVE(kind_name, kind_val, mode_name, mode_val)          \
  extern "C" _LIBCPP_EXPORTED_FROM_ABI void                                     \
  __cxa_contract_violation_##kind_name##_noexcept_observe_##mode_name##_noexcept \
      (void* __data) noexcept {                                                \
    __nx_override(__data, kind_val, CXA_ES_NOEXCEPT_OBSERVE, mode_val);        \
  }

CXA_NOEXCEPT_OBSERVE(pre,          CXA_AK_PRE,          pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(pre,          CXA_AK_PRE,          ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(post,         CXA_AK_POST,         pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(post,         CXA_AK_POST,         ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(assert,       CXA_AK_ASSERT,       pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(assert,       CXA_AK_ASSERT,       ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(post_capture, CXA_AK_POST_CAPTURE, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(implicit,     CXA_AK_IMPLICIT,     ex, CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_NOEXCEPT_OBSERVE

// Specialized entry points: noexcept_enforce ([[noreturn]], D4298).
#define CXA_NOEXCEPT_ENFORCE(kind_name, kind_val, mode_name, mode_val)          \
  extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void                        \
  __cxa_contract_violation_##kind_name##_noexcept_enforce_##mode_name##_noexcept \
      (void* __data) noexcept {                                                \
    __nx_override(__data, kind_val, CXA_ES_NOEXCEPT_ENFORCE, mode_val);        \
    __builtin_unreachable();                                                   \
  }

CXA_NOEXCEPT_ENFORCE(pre,          CXA_AK_PRE,          pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(pre,          CXA_AK_PRE,          ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(post,         CXA_AK_POST,         pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(post,         CXA_AK_POST,         ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(assert,       CXA_AK_ASSERT,       pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(assert,       CXA_AK_ASSERT,       ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(post_capture, CXA_AK_POST_CAPTURE, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(implicit,     CXA_AK_IMPLICIT,     ex, CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_NOEXCEPT_ENFORCE

// P3100: pure-virtual-call terminus variants (ub:class.abstract.pure.virtual).
//
// The vtable slot for a pure virtual is a plain void() function pointer.  When
// the class's implicit contract configuration (resolved where the vtable is
// emitted) selects a checking semantic, the compiler points that slot at one of
// these instead of the legacy __cxa_pure_virtual.  Each builds a generic
// implicit contract-violation data block ("pure virtual function called", with
// no per-site source location -- the terminus is shared across every pure
// virtual configured to this semantic) and dispatches it through the
// contract-violation handler.
//
// A pure-virtual call has no valid continuation (no function to run, no value to
// return), so every reacting variant ends by terminating; the one escape is a
// throwing handler on a non-noexcept pure virtual, which unwinds out through the
// caller.  The compiler selects the _noexcept variant when the pure virtual is
// declared noexcept, so such a throwing handler terminates here at the noexcept
// boundary rather than escaping into a caller that assumed the call could not
// throw.

namespace {

// A minimal data block matching the compiler-emitted layout: the five basic
// fields an implicit violation needs (no local handler, no lazy report).
struct __pv_data_block {
  const __cxa_descriptor_table_t*  descriptor;
  const __cxa_contract_data_block* next;
  __cxa_source_location            location;
  const char*                      comment;
  __UINT8_TYPE__                   kind;
  __UINT8_TYPE__                   semantic;
  __UINT8_TYPE__                   mode;
};

struct __pv_desc {
  __UINT8_TYPE__          header;
  __UINT8_TYPE__          num_entries;
  __UINT8_TYPE__          fid[5];
  __UINT8_TYPE__          pad[1];
  __cxa_descriptor_data_t data[5];
};

const __pv_desc __pv_descriptor = {
    (__UINT8_TYPE__)((1u << 4) | CXA_VENDOR_CLANG),
    5,
    {CXA_FIELD_SOURCE_LOCATION, CXA_FIELD_COMMENT, CXA_FIELD_ASSERTION_KIND,
     CXA_FIELD_EVALUATION_SEMANTIC, CXA_FIELD_DETECTION_MODE},
    {0},
    {
        {__builtin_offsetof(__pv_data_block, location)},
        {__builtin_offsetof(__pv_data_block, comment)},
        {__builtin_offsetof(__pv_data_block, kind)},
        {__builtin_offsetof(__pv_data_block, semantic)},
        {__builtin_offsetof(__pv_data_block, mode)},
    }};

// Fill DATA with a generic pure-virtual implicit violation carrying SEM.
void __pv_fill(__pv_data_block& __data, __UINT8_TYPE__ __sem) {
  __data.descriptor          = (const __cxa_descriptor_table_t*)&__pv_descriptor;
  __data.next                = nullptr;
  __data.location.file_name  = "";
  __data.location.function_name = "";
  __data.location.line       = 0;
  __data.location.column     = 0;
  __data.comment             = "pure virtual function called";
  __data.kind                = (__UINT8_TYPE__)CXA_AK_IMPLICIT;
  __data.semantic            = __sem;
  __data.mode                = (__UINT8_TYPE__)CXA_DM_PREDICATE_FALSE;
}

} // namespace

// quick_enforce: fast, silent termination -- no handler, no report.
extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_pure_virtual_quick (void) noexcept {
  __builtin_trap();
}

// enforce (throwing): the handler runs; a throwing handler unwinds out through
// this frame to the caller, otherwise the enforcing core terminates via abort()
// after the handler returns.
extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_pure_virtual_enforce (void) {
  __pv_data_block __data;
  __pv_fill(__data, (__UINT8_TYPE__)CXA_ES_ENFORCE);
  __contract_dispatch_core((const __cxa_contract_data_block*)&__data,
                           (__UINT8_TYPE__)CXA_ES_ENFORCE);
  __builtin_unreachable();
}

// noexcept_enforce: as enforce, but a throwing handler terminates here (the pure
// virtual is noexcept, so it must not escape into the caller).
extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_pure_virtual_noexcept_enforce (void) noexcept {
  __pv_data_block __data;
  __pv_fill(__data, (__UINT8_TYPE__)CXA_ES_NOEXCEPT_ENFORCE);
#if _LIBCPP_HAS_EXCEPTIONS
  try {
    __contract_dispatch_core((const __cxa_contract_data_block*)&__data,
                             (__UINT8_TYPE__)CXA_ES_NOEXCEPT_ENFORCE);
  } catch (...) {
    std::terminate();
  }
#else
  __contract_dispatch_core((const __cxa_contract_data_block*)&__data,
                           (__UINT8_TYPE__)CXA_ES_NOEXCEPT_ENFORCE);
#endif
  __builtin_unreachable();
}

// observe (throwing): the handler runs and returns; a throwing handler unwinds
// out through this frame to the caller.  A pure-virtual call has no valid
// continuation, so if the handler returns normally we terminate the same way the
// enforcing core does (abort()).
extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_pure_virtual_observe (void) {
  __pv_data_block __data;
  __pv_fill(__data, (__UINT8_TYPE__)CXA_ES_OBSERVE);
  __contract_dispatch_core((const __cxa_contract_data_block*)&__data,
                           (__UINT8_TYPE__)CXA_ES_OBSERVE);
  __builtin_abort();
}

// noexcept_observe: as observe, but a throwing handler terminates here.
extern "C" [[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__cxa_pure_virtual_noexcept_observe (void) noexcept {
  __pv_data_block __data;
  __pv_fill(__data, (__UINT8_TYPE__)CXA_ES_NOEXCEPT_OBSERVE);
#if _LIBCPP_HAS_EXCEPTIONS
  try {
    __contract_dispatch_core((const __cxa_contract_data_block*)&__data,
                             (__UINT8_TYPE__)CXA_ES_NOEXCEPT_OBSERVE);
  } catch (...) {
    std::terminate();
  }
#else
  __contract_dispatch_core((const __cxa_contract_data_block*)&__data,
                           (__UINT8_TYPE__)CXA_ES_NOEXCEPT_OBSERVE);
#endif
  __builtin_abort();
}
