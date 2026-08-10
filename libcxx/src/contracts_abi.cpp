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
