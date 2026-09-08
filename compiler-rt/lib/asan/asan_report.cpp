//===-- asan_report.cpp ---------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// This file contains error reporting code.
//===----------------------------------------------------------------------===//

#include "asan_report.h"

#include "asan_descriptions.h"
#include "asan_errors.h"
#include "asan_flags.h"
#include "asan_internal.h"
#include "asan_mapping.h"
#include "asan_scariness_score.h"
#include "asan_stack.h"
#include "asan_thread.h"
#include "lsan/lsan_common.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_file.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_interface_internal.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_placement_new.h"
#include "sanitizer_common/sanitizer_report_decorator.h"
#include "sanitizer_common/sanitizer_stackdepot.h"
#include "sanitizer_common/sanitizer_symbolizer.h"

namespace __asan {

// -------------------- User-specified callbacks ----------------- {{{1
static void (*error_report_callback)(const char*);
using ErrorMessageBuffer = InternalMmapVectorNoCtor<char, true>;
alignas(
    alignof(ErrorMessageBuffer)) static char error_message_buffer_placeholder
    [sizeof(ErrorMessageBuffer)];
static ErrorMessageBuffer *error_message_buffer = nullptr;
static Mutex error_message_buf_mutex;
static const unsigned kAsanBuggyPcPoolSize = 25;
static __sanitizer::atomic_uintptr_t AsanBuggyPcPool[kAsanBuggyPcPoolSize];

// -------------------- P3100 contract-routing ------------------- {{{1
//
// Task 2.1 (CL2): the compiler emits a per-TU weak byte
// __asan_contract_semantic when -fcontracts-p3100 routes the address check to
// the contract-violation handler.  Wire encoding: 0 = stock (routing off /
// symbol absent), 1 = observe (report + call handler + continue), 2 = enforce
// (report + call handler + terminate), 3 = quick_enforce (report + terminate
// WITHOUT calling the handler).  We declare it weak so a non-p3100 program (no
// such symbol) reads as 0 = stock and the runtime keeps its existing behavior
// untouched.
//
// Task 4.1: the handler is invoked below from inside this file's implicitly-
// noexcept ScopedInErrorReport destructor, so a throwing handler can never
// propagate -- it would hit "exception escaping a noexcept function" and
// std::terminate() several frames below any user catch/RAII.  That is why the
// compiler only ever routes the NON-throwing semantics here: observe/enforce
// on the wire are really the D4298 noexcept_observe/noexcept_enforce paths
// (gated on -fcontracts-p4298), and quick_enforce terminates without ever
// entering the handler.
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char __asan_contract_semantic;

// Option 2 (pointer-pair checks): the two ASan checks that report an invalid
// pointer pair -- pointer-compare ([expr.rel]) and pointer-subtract
// (expr.add.sub.diff.pointers) -- flow through the SAME ScopedInErrorReport
// path as ordinary address errors, but are governed by their OWN wire bytes so
// the address routing scope stays exactly as-is.  Absent symbol reads as stock.
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char
    __asan_contract_semantic_pointer_compare;
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char
    __asan_contract_semantic_pointer_subtract;

enum {
  kAsanContractStock = 0,
  kAsanContractObserve = 1,
  kAsanContractEnforce = 2,
  kAsanContractQuick = 3,
};

// Which routed check a report belongs to -- selects which wire byte governs it.
// kAsanCheckAddress covers every ordinary ASan error (the address byte); the
// pointer-pair reports select their own byte.  All existing report sites use
// the default, so their routing is unchanged.
enum AsanContractCheck {
  kAsanCheckAddress = 0,
  kAsanCheckPointerCompare,
  kAsanCheckPointerSubtract,
};

// Return the conveyed semantic for CHECK (0 = stock when its weak descriptor is
// absent).
static unsigned char AsanContractSemantic(
    AsanContractCheck check = kAsanCheckAddress) {
  switch (check) {
    case kAsanCheckPointerCompare:
      if (&__asan_contract_semantic_pointer_compare == nullptr)
        return kAsanContractStock;
      return __asan_contract_semantic_pointer_compare;
    case kAsanCheckPointerSubtract:
      if (&__asan_contract_semantic_pointer_subtract == nullptr)
        return kAsanContractStock;
      return __asan_contract_semantic_pointer_subtract;
    case kAsanCheckAddress:
      break;
  }
  if (&__asan_contract_semantic == nullptr)
    return kAsanContractStock;
  return __asan_contract_semantic;
}

// The lazy report populator ABI struct (mirror of
// __cxa_contract_report_populator in libcontracts/contracts-abi.h and libc++
// __contracts/abi.h).  We redeclare it here rather than including the C++
// runtime header so this file stays free-standing; the layout must match {
// const char* (*)(const void*), const void* }.
struct AsanContractReportPopulator {
  const char* (*populate)(const void* ctx);
  const void* ctx;
};

// Task 2.2 / RF5: the contract-violation report leg, provided by the C++
// runtime (libc++).  Declared weak: if the C++ contracts runtime is not linked
// the symbol is absent, and we must fall back to stock behavior rather than
// call a null pointer.  Builds an implicit contract_violation and invokes the
// handler; always returns (termination for enforce is performed here by us).
// The final argument is an optional lazy report populator (CXA_FIELD_REPORT):
// the handler calls contract_violation::report(), which invokes populate(ctx)
// on demand.
extern "C" SANITIZER_WEAK_ATTRIBUTE void __cxa_contract_violation_sanitizer(
    const char* comment, const char* file, unsigned line,
    unsigned char semantic, const AsanContractReportPopulator* report);

// RF5: lazy populator context.  Caches the rendered report so repeat report()
// calls within one handler invocation are cheap.  Lives on the dtor stack frame
// (still alive for the whole handler call), and references the still-live
// current error via ScopedInErrorReport::CurrentError().
struct AsanContractReportCtx {
  const char* rendered;  // nullptr until first populate(); then producer-owned.
};

// Renders the current ASan error into a NUL-terminated, producer-owned buffer
// WITHOUT writing to stderr / the report fd (RF1 mechanism).  Returns the
// cached pointer on repeat calls.  Only ever runs when the handler calls
// report().
static const char* asan_contract_report_populate(const void* ctx);

void AppendToErrorMessageBuffer(const char *buffer) {
  Lock l(&error_message_buf_mutex);
  if (!error_message_buffer) {
    error_message_buffer =
        new (error_message_buffer_placeholder) ErrorMessageBuffer();
    error_message_buffer->Initialize(kErrorMessageBufferSize);
  }
  uptr error_message_buffer_len = error_message_buffer->size();
  uptr buffer_len = internal_strlen(buffer);
  error_message_buffer->resize(error_message_buffer_len + buffer_len);
  internal_memcpy(error_message_buffer->data() + error_message_buffer_len,
                  buffer, buffer_len);
}

// ---------------------- Helper functions ----------------------- {{{1

void PrintMemoryByte(InternalScopedString *str, const char *before, u8 byte,
                     bool in_shadow, const char *after) {
  Decorator d;
  str->AppendF("%s%s%x%x%s%s", before,
               in_shadow ? d.ShadowByte(byte) : d.MemoryByte(), byte >> 4,
               byte & 15, d.Default(), after);
}

static void PrintZoneForPointer(uptr ptr, uptr zone_ptr,
                                const char *zone_name) {
  if (zone_ptr) {
    if (zone_name) {
      Printf("malloc_zone_from_ptr(%p) = %p, which is %s\n", (void *)ptr,
             (void *)zone_ptr, zone_name);
    } else {
      Printf("malloc_zone_from_ptr(%p) = %p, which doesn't have a name\n",
             (void *)ptr, (void *)zone_ptr);
    }
  } else {
    Printf("malloc_zone_from_ptr(%p) = 0\n", (void *)ptr);
  }
}

// ---------------------- Address Descriptions ------------------- {{{1

bool ParseFrameDescription(const char *frame_descr,
                           InternalMmapVector<StackVarDescr> *vars) {
  CHECK(frame_descr);
  const char *p;
  // This string is created by the compiler and has the following form:
  // "n alloc_1 alloc_2 ... alloc_n"
  // where alloc_i looks like "offset size len ObjectName"
  // or                       "offset size len ObjectName:line".
  uptr n_objects = (uptr)internal_simple_strtoll(frame_descr, &p, 10);
  if (n_objects == 0)
    return false;

  for (uptr i = 0; i < n_objects; i++) {
    uptr beg  = (uptr)internal_simple_strtoll(p, &p, 10);
    uptr size = (uptr)internal_simple_strtoll(p, &p, 10);
    uptr len  = (uptr)internal_simple_strtoll(p, &p, 10);
    if (beg == 0 || size == 0 || *p != ' ') {
      return false;
    }
    p++;
    char *colon_pos = internal_strchr(p, ':');
    uptr line = 0;
    uptr name_len = len;
    if (colon_pos != nullptr && colon_pos < p + len) {
      name_len = colon_pos - p;
      line = (uptr)internal_simple_strtoll(colon_pos + 1, nullptr, 10);
    }
    StackVarDescr var = {beg, size, p, name_len, line};
    vars->push_back(var);
    p += len;
  }

  return true;
}

// -------------------- Different kinds of reports ----------------- {{{1

// Use ScopedInErrorReport to run common actions just before and
// immediately after printing error report.
class ScopedInErrorReport {
 public:
  explicit ScopedInErrorReport(
      bool fatal = false, AsanContractCheck contract_check = kAsanCheckAddress)
      : halt_on_error_(fatal || flags()->halt_on_error),
        contract_check_(contract_check) {
    // Deadlock Prevention Between ASan and LSan
    //
    // Background:
    // - The `dl_iterate_phdr` function requires holding libdl's internal lock
    //   (Lock A).
    // - LSan acquires the ASan thread registry lock (Lock B) *after* calling
    //   `dl_iterate_phdr`.
    //
    // Problem Scenario:
    // When ASan attempts to call `dl_iterate_phdr` while holding Lock B (e.g.,
    // during error reporting via `ErrorDescription::Print`), a circular lock
    // dependency may occur:
    //   1. Thread 1: Holds Lock B → Requests Lock A (via dl_iterate_phdr)
    //   2. Thread 2: Holds Lock A → Requests Lock B (via LSan operations)
    //
    // Solution:
    // Proactively load all required modules before acquiring Lock B.
    // This ensures:
    // 1. Any `dl_iterate_phdr` calls during module loading complete before
    //    locking.
    // 2. Subsequent error reporting avoids nested lock acquisition patterns.
    // 3. Eliminates the lock order inversion risk between libdl and ASan's
    //    thread registry.
#if CAN_SANITIZE_LEAKS && (SANITIZER_LINUX || SANITIZER_NETBSD)
    Symbolizer::GetOrInit()->GetRefreshedListOfModules();
#endif

    // Make sure the registry and sanitizer report mutexes are locked while
    // we're printing an error report.
    // We can lock them only here to avoid self-deadlock in case of
    // recursive reports.
    asanThreadRegistry().Lock();
    // RF5: on the contract-routed path (observe/enforce/quick) the handler owns
    // all output -- the sanitizer must emit NOTHING before it.  This "=====\n"
    // banner is the first Printf of any report, so suppress it when routing is
    // active.  Stock behavior (routing off) is byte-for-byte unchanged.
    if (AsanContractSemantic(contract_check_) == kAsanContractStock)
      Printf(
          "================================================================="
          "\n");
  }

  ~ScopedInErrorReport() {
    // P3100 Task 2.2 / Task 4.1 / RF5: decide whether this report is
    // contract-routed and, if so, whether the handler is invoked.
    // observe/enforce (wire 1/2) call the handler and require the C++ runtime's
    // report entry point to be linked.  quick_enforce (wire 3) terminates
    // WITHOUT calling the handler, so it is active independently of whether
    // that entry point is present.  When routing is off, every branch below is
    // byte-for-byte stock behavior.
    const unsigned char kContractRoute = AsanContractSemantic(contract_check_);
    const bool contract_handler_linked =
        (&__cxa_contract_violation_sanitizer != nullptr);
    const bool contract_route_handler =
        (kContractRoute == kAsanContractObserve ||
         kContractRoute == kAsanContractEnforce) &&
        contract_handler_linked;
    const bool contract_route_quick = (kContractRoute == kAsanContractQuick);
    const bool contract_routed = (kContractRoute != kAsanContractStock);

    // P3100 Bug #3: on the contract-routed path the configured semantic ALONE
    // decides behavior -- it must not depend on ASAN_OPTIONS.  So skip the
    // halt_on_error_ / __sanitizer_acquire_crash_state() gate here when routing
    // is active: consuming the process-wide crash-state latch would make a
    // later genuine report early-return without resetting current_error_ (and
    // without invoking the routed handler), and halt_on_error_ itself is
    // flags()->halt_on_error-driven.  The routed branches below terminate or
    // continue purely per the semantic; the stock path (routing off) keeps this
    // gate byte-for-byte.  abort_on_error / halt_on_error_ in the tail are only
    // reached on the stock path (the routed branches return/Die first).
    if (!contract_routed && halt_on_error_ &&
        !__sanitizer_acquire_crash_state()) {
      asanThreadRegistry().Unlock();
      return;
    }
    ASAN_ON_ERROR();
    // Capture a description of the error before printing (the error object may
    // be reset below on the continue path).
    const char* contract_comment =
        current_error_.IsValid()
            ? current_error_.Base.scariness.GetDescription()
            : "address-sanitizer-error";
    // Name the specific UB for the pointer-pair checks so the handler's comment
    // identifies which check fired ([expr.rel] vs expr.add.sub.diff.pointers)
    // rather than the generic scariness description.
    if (contract_check_ == kAsanCheckPointerCompare)
      contract_comment = "pointer-compare";
    else if (contract_check_ == kAsanCheckPointerSubtract)
      contract_comment = "pointer-subtract";

    // RF5: on the contract-routed path (observe/enforce/quick) the handler owns
    // ALL output.  Emit NOTHING here -- no current_error_.Print(), no
    // DescribeThread, no stats, no LogFullErrorReport, no stock
    // error_report_callback.  Instead, for the handler routes, register a lazy
    // report populator so the handler's contract_violation::report() renders
    // the full ASan text on demand (and only if it calls report()).
    // quick_enforce terminates silently with no populator.  Every non-routed
    // path below is byte-for-byte stock behavior.
    if (contract_route_quick) {
      // quick_enforce = terminate silently, no handler, no report, no output.
      asanThreadRegistry().Unlock();
      Die();
    }
    if (contract_route_handler) {
      // The populator ctx lives on this (still-alive) frame; the error object
      // stays valid until we reset/Die below, so the populator can render it
      // during the handler call.  Keep asanThreadRegistry() LOCKED across the
      // handler: the RF1 render (ErrorDescription::Print / DescribeThread)
      // CheckLocked()s the registry, so the populator must run with it held.
      // The enclosing ScopedErrorReportLock already serializes all reporting,
      // so no new deadlock surface is introduced.
      AsanContractReportCtx report_ctx = {/*rendered=*/nullptr};
      AsanContractReportPopulator report_populator = {
          &asan_contract_report_populate, &report_ctx};
      __cxa_contract_violation_sanitizer(contract_comment, /*file=*/"",
                                         /*line=*/0, kContractRoute,
                                         &report_populator);
      asanThreadRegistry().Unlock();
      if (kContractRoute == kAsanContractObserve) {
        // noexcept_observe = continue: reset the error object and return
        // without terminating, regardless of halt_on_error_.
        internal_memset(&current_error_, 0, sizeof(current_error_));
        return;
      }
      // noexcept_enforce = terminate.  No "ABORTING" line: the handler owns all
      // output on the routed path.
      Die();
    }

    if (current_error_.IsValid()) current_error_.Print();

    // Make sure the current thread is announced.
    DescribeThread(GetCurrentThread());
    // We may want to grab this lock again when printing stats.
    asanThreadRegistry().Unlock();
    // Print memory stats.
    if (flags()->print_stats)
      __asan_print_accumulated_stats();

    if (common_flags()->print_cmdline)
      PrintCmdline();

    if (common_flags()->print_module_map == 2)
      DumpProcessMap();

    // Copy the message buffer so that we could start logging without holding a
    // lock that gets acquired during printing.
    InternalScopedString buffer_copy;
    {
      Lock l(&error_message_buf_mutex);
      error_message_buffer->push_back('\0');
      buffer_copy.Append(error_message_buffer->data());
      // Clear error_message_buffer so that if we find other errors
      // we don't re-log this error.
      error_message_buffer->clear();
    }

    LogFullErrorReport(buffer_copy.data());

    if (error_report_callback) {
      error_report_callback(buffer_copy.data());
    }

    if (halt_on_error_ && common_flags()->abort_on_error) {
      // On Android the message is truncated to 512 characters.
      // FIXME: implement "compact" error format, possibly without, or with
      // highly compressed stack traces?
      // FIXME: or just use the summary line as abort message?
      SetAbortMessage(buffer_copy.data());
    }

    // In halt_on_error = false mode, reset the current error object (before
    // unlocking).
    if (!halt_on_error_)
      internal_memset(&current_error_, 0, sizeof(current_error_));

    if (halt_on_error_) {
      Report("ABORTING\n");
      Die();
    }
  }

  void ReportError(const ErrorDescription &description) {
    // Can only report one error per ScopedInErrorReport.
    CHECK_EQ(current_error_.kind, kErrorKindInvalid);
    internal_memcpy(&current_error_, &description, sizeof(current_error_));
  }

  static ErrorDescription &CurrentError() {
    return current_error_;
  }

 private:
  ScopedErrorReportLock error_report_lock_;
  // Error currently being reported. This enables the destructor to interact
  // with the debugger and point it to an error description.
  static ErrorDescription current_error_;
  bool halt_on_error_;
  // Which routed check governs this report (selects the wire byte).  Defaults
  // to the address byte, so ordinary ASan reports are unchanged.
  AsanContractCheck contract_check_;
};

ErrorDescription ScopedInErrorReport::current_error_(LINKER_INITIALIZED);

// RF5 lazy report populator (RF1 buffer-only render mechanism).  Renders the
// current ASan error into the file-local error_message_buffer with sink 1 (the
// report fd) temporarily redirected to /dev/null, so nothing reaches stderr
// where that redirect is available (see below);
// then copies the accumulated text out NUL-terminated into producer-owned
// storage.  Runs only when the contract handler calls report(); the result is
// cached in the ctx so repeat calls within one handler invocation are cheap.
//
// This is invoked with asanThreadRegistry() held (see the routed-handler path
// in the dtor), which ErrorDescription::Print()/DescribeThread require.
static const char* asan_contract_report_populate(const void* ctx_v) {
  AsanContractReportCtx* ctx = const_cast<AsanContractReportCtx*>(
      static_cast<const AsanContractReportCtx*>(ctx_v));
  if (!ctx)
    return nullptr;
  if (ctx->rendered)
    return ctx->rendered;

  if (!ScopedInErrorReport::CurrentError().IsValid()) {
    ctx->rendered = "";
    return ctx->rendered;
  }

  // Redirect the report fd (sink 1) to /dev/null around Print() so the fully
  // rendered report only accumulates into error_message_buffer (sink 2).
  //
  // Where "/dev/null" does not name the null device, OpenFile returns
  // kInvalidFd and every use below is guarded, so the redirect is simply
  // skipped: the report still renders into the buffer, but it also reaches the
  // report fd.  Degraded rather than broken, and the alternative -- a
  // platform-specific null-device name -- is not worth carrying here.
  fd_t devnull = OpenFile("/dev/null", WrOnly);
  fd_t saved_fd;
  uptr saved_fd_pid;
  {
    SpinMutexLock l(report_file.mu);
    saved_fd = report_file.fd;
    saved_fd_pid = report_file.fd_pid;
    if (devnull != kInvalidFd) {
      report_file.fd = devnull;
      // Match fd_pid to our pid so ReopenIfNecessary() won't clobber the swap.
      report_file.fd_pid = internal_getpid();
    }
  }

  // Clear the buffer, render (sink 1 -> /dev/null, sink 2 -> buffer), then
  // copy.
  const char* result = "";
  {
    Lock bl(&error_message_buf_mutex);
    if (error_message_buffer)
      error_message_buffer->clear();
  }
  ScopedInErrorReport::CurrentError().Print();

  // Restore the report fd immediately after rendering.
  {
    SpinMutexLock l(report_file.mu);
    report_file.fd = saved_fd;
    report_file.fd_pid = saved_fd_pid;
  }
  if (devnull != kInvalidFd)
    CloseFile(devnull);

  // Copy the accumulated text out NUL-terminated into producer-owned storage.
  // A single static buffer suffices: all error reporting is serialized by the
  // enclosing ScopedErrorReportLock, so only one populator runs at a time, and
  // the rendered string must stay valid only for the current handler call.
  static char rendered_report[kErrorMessageBufferSize];
  {
    Lock bl(&error_message_buf_mutex);
    if (error_message_buffer && error_message_buffer->size()) {
      uptr n = error_message_buffer->size();
      if (n >= sizeof(rendered_report))
        n = sizeof(rendered_report) - 1;
      internal_memcpy(rendered_report, error_message_buffer->data(), n);
      rendered_report[n] = '\0';
      error_message_buffer->clear();
      result = rendered_report;
    }
  }

  ctx->rendered = result;
  return ctx->rendered;
}

void ReportDeadlySignal(const SignalContext &sig) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorDeadlySignal error(GetCurrentTidOrInvalid(), sig);
  in_report.ReportError(error);
}

void ReportDoubleFree(uptr addr, BufferedStackTrace *free_stack) {
  ScopedInErrorReport in_report;
  ErrorDoubleFree error(GetCurrentTidOrInvalid(), free_stack, addr);
  in_report.ReportError(error);
}

void ReportNewDeleteTypeMismatch(uptr addr, uptr delete_size,
                                 uptr delete_alignment,
                                 BufferedStackTrace *free_stack) {
  ScopedInErrorReport in_report;
  ErrorNewDeleteTypeMismatch error(GetCurrentTidOrInvalid(), free_stack, addr,
                                   delete_size, delete_alignment);
  in_report.ReportError(error);
}

void ReportFreeSizeMismatch(uptr addr, uptr delete_size, uptr delete_alignment,
                            BufferedStackTrace* free_stack) {
  ScopedInErrorReport in_report;
  ErrorFreeSizeMismatch error(GetCurrentTidOrInvalid(), free_stack, addr,
                              delete_size, delete_alignment);
  in_report.ReportError(error);
}

void ReportFreeNotMalloced(uptr addr, BufferedStackTrace *free_stack) {
  ScopedInErrorReport in_report;
  ErrorFreeNotMalloced error(GetCurrentTidOrInvalid(), free_stack, addr);
  in_report.ReportError(error);
}

void ReportAllocTypeMismatch(uptr addr, BufferedStackTrace *free_stack,
                             AllocType alloc_type,
                             AllocType dealloc_type) {
  ScopedInErrorReport in_report;
  ErrorAllocTypeMismatch error(GetCurrentTidOrInvalid(), free_stack, addr,
                               alloc_type, dealloc_type);
  in_report.ReportError(error);
}

void ReportMallocUsableSizeNotOwned(uptr addr, BufferedStackTrace *stack) {
  ScopedInErrorReport in_report;
  ErrorMallocUsableSizeNotOwned error(GetCurrentTidOrInvalid(), stack, addr);
  in_report.ReportError(error);
}

void ReportSanitizerGetAllocatedSizeNotOwned(uptr addr,
                                             BufferedStackTrace *stack) {
  ScopedInErrorReport in_report;
  ErrorSanitizerGetAllocatedSizeNotOwned error(GetCurrentTidOrInvalid(), stack,
                                               addr);
  in_report.ReportError(error);
}

void ReportCallocOverflow(uptr count, uptr size, BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorCallocOverflow error(GetCurrentTidOrInvalid(), stack, count, size);
  in_report.ReportError(error);
}

void ReportReallocArrayOverflow(uptr count, uptr size,
                                BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorReallocArrayOverflow error(GetCurrentTidOrInvalid(), stack, count, size);
  in_report.ReportError(error);
}

void ReportPvallocOverflow(uptr size, BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorPvallocOverflow error(GetCurrentTidOrInvalid(), stack, size);
  in_report.ReportError(error);
}

void ReportInvalidAllocationAlignment(uptr alignment,
                                      BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorInvalidAllocationAlignment error(GetCurrentTidOrInvalid(), stack,
                                        alignment);
  in_report.ReportError(error);
}

void ReportInvalidAlignedAllocAlignment(uptr size, uptr alignment,
                                        BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorInvalidAlignedAllocAlignment error(GetCurrentTidOrInvalid(), stack,
                                          size, alignment);
  in_report.ReportError(error);
}

void ReportInvalidPosixMemalignAlignment(uptr alignment,
                                         BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorInvalidPosixMemalignAlignment error(GetCurrentTidOrInvalid(), stack,
                                           alignment);
  in_report.ReportError(error);
}

void ReportAllocationSizeTooBig(uptr user_size, uptr total_size, uptr max_size,
                                BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorAllocationSizeTooBig error(GetCurrentTidOrInvalid(), stack, user_size,
                                  total_size, max_size);
  in_report.ReportError(error);
}

void ReportRssLimitExceeded(BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorRssLimitExceeded error(GetCurrentTidOrInvalid(), stack);
  in_report.ReportError(error);
}

void ReportOutOfMemory(uptr requested_size, BufferedStackTrace *stack) {
  ScopedInErrorReport in_report(/*fatal*/ true);
  ErrorOutOfMemory error(GetCurrentTidOrInvalid(), stack, requested_size);
  in_report.ReportError(error);
}

void ReportStringFunctionMemoryRangesOverlap(const char *function,
                                             const char *offset1, uptr length1,
                                             const char *offset2, uptr length2,
                                             BufferedStackTrace *stack) {
  ScopedInErrorReport in_report;
  ErrorStringFunctionMemoryRangesOverlap error(
      GetCurrentTidOrInvalid(), stack, (uptr)offset1, length1, (uptr)offset2,
      length2, function);
  in_report.ReportError(error);
}

void ReportStringFunctionSizeOverflow(uptr offset, uptr size, bool is_write,
                                      BufferedStackTrace* stack) {
  ScopedInErrorReport in_report;
  ErrorStringFunctionSizeOverflow error(GetCurrentTidOrInvalid(), stack, offset,
                                        size, is_write);
  in_report.ReportError(error);
}

void ReportBadParamsToAnnotateContiguousContainer(uptr beg, uptr end,
                                                  uptr old_mid, uptr new_mid,
                                                  BufferedStackTrace *stack) {
  ScopedInErrorReport in_report;
  ErrorBadParamsToAnnotateContiguousContainer error(
      GetCurrentTidOrInvalid(), stack, beg, end, old_mid, new_mid);
  in_report.ReportError(error);
}

void ReportBadParamsToAnnotateDoubleEndedContiguousContainer(
    uptr storage_beg, uptr storage_end, uptr old_container_beg,
    uptr old_container_end, uptr new_container_beg, uptr new_container_end,
    BufferedStackTrace *stack) {
  ScopedInErrorReport in_report;
  ErrorBadParamsToAnnotateDoubleEndedContiguousContainer error(
      GetCurrentTidOrInvalid(), stack, storage_beg, storage_end,
      old_container_beg, old_container_end, new_container_beg,
      new_container_end);
  in_report.ReportError(error);
}

void ReportODRViolation(const __asan_global *g1, u32 stack_id1,
                        const __asan_global *g2, u32 stack_id2) {
  ScopedInErrorReport in_report;
  ErrorODRViolation error(GetCurrentTidOrInvalid(), g1, stack_id1, g2,
                          stack_id2);
  in_report.ReportError(error);
}

// ----------------------- CheckForInvalidPointerPair ----------- {{{1
static NOINLINE void ReportInvalidPointerPair(
    uptr pc, uptr bp, uptr sp, uptr a1, uptr a2,
    AsanContractCheck contract_check) {
  ScopedInErrorReport in_report(/*fatal=*/false, contract_check);
  ErrorInvalidPointerPair error(GetCurrentTidOrInvalid(), pc, bp, sp, a1, a2);
  in_report.ReportError(error);
}

static bool IsInvalidPointerPair(uptr a1, uptr a2) {
  if (a1 == a2)
    return false;

  // 256B in shadow memory can be iterated quite fast
  static const uptr kMaxOffset = 2048;

  uptr left = a1 < a2 ? a1 : a2;
  uptr right = a1 < a2 ? a2 : a1;
  uptr offset = right - left;
  if (offset <= kMaxOffset)
    return __asan_region_is_poisoned(left, offset);

  AsanThread *t = GetCurrentThread();

  // check whether left is a stack memory pointer
  if (uptr shadow_offset1 = t->GetStackVariableShadowStart(left)) {
    uptr shadow_offset2 = t->GetStackVariableShadowStart(right);
    return shadow_offset2 == 0 || shadow_offset1 != shadow_offset2;
  }

  // check whether left is a heap memory address
  HeapAddressDescription hdesc1, hdesc2;
  if (GetHeapAddressInformation(left, 0, &hdesc1) &&
      hdesc1.chunk_access.access_type == kAccessTypeInside)
    return !GetHeapAddressInformation(right, 0, &hdesc2) ||
        hdesc2.chunk_access.access_type != kAccessTypeInside ||
        hdesc1.chunk_access.chunk_begin != hdesc2.chunk_access.chunk_begin;

  // check whether left is an address of a global variable
  GlobalAddressDescription gdesc1, gdesc2;
  if (GetGlobalAddressInformation(left, 0, &gdesc1))
    return !GetGlobalAddressInformation(right - 1, 0, &gdesc2) ||
        !gdesc1.PointsInsideTheSameVariable(gdesc2);

  if (t->GetStackVariableShadowStart(right) ||
      GetHeapAddressInformation(right, 0, &hdesc2) ||
      GetGlobalAddressInformation(right - 1, 0, &gdesc2))
    return true;

  // At this point we know nothing about both a1 and a2 addresses.
  return false;
}

static inline void CheckForInvalidPointerPair(
    void* p1, void* p2, AsanContractCheck contract_check) {
  switch (flags()->detect_invalid_pointer_pairs) {
    case 0:
      return;
    case 1:
      if (p1 == nullptr || p2 == nullptr)
        return;
      break;
  }

  uptr a1 = reinterpret_cast<uptr>(p1);
  uptr a2 = reinterpret_cast<uptr>(p2);

  if (IsInvalidPointerPair(a1, a2)) {
    GET_CALLER_PC_BP_SP;
    ReportInvalidPointerPair(pc, bp, sp, a1, a2, contract_check);
  }
}
// ----------------------- Mac-specific reports ----------------- {{{1

void ReportMacMzReallocUnknown(uptr addr, uptr zone_ptr, const char *zone_name,
                               BufferedStackTrace *stack) {
  ScopedInErrorReport in_report;
  Printf(
      "mz_realloc(%p) -- attempting to realloc unallocated memory.\n"
      "This is an unrecoverable problem, exiting now.\n",
      (void *)addr);
  PrintZoneForPointer(addr, zone_ptr, zone_name);
  stack->Print();
  DescribeAddressIfHeap(addr);
}

// -------------- SuppressErrorReport -------------- {{{1
// Avoid error reports duplicating for ASan recover mode.
static bool SuppressErrorReport(uptr pc) {
  // P3100 Bug #3: on the contract-routed path, per-PC dedup is applied
  // DETERMINISTICALLY (report once per site; subsequent same-PC violations
  // continue silently, per "continue as if the check were not present"),
  // regardless of the suppress_equal_pcs flag -- so behavior depends only on
  // the configured semantic, not ASAN_OPTIONS.  Off the routed path the flag is
  // honored exactly as before.
  const bool routed = (AsanContractSemantic() != kAsanContractStock);
  if (!routed && !common_flags()->suppress_equal_pcs)
    return false;
  for (unsigned i = 0; i < kAsanBuggyPcPoolSize; i++) {
    uptr cmp = atomic_load_relaxed(&AsanBuggyPcPool[i]);
    if (cmp == 0 && atomic_compare_exchange_strong(&AsanBuggyPcPool[i], &cmp,
                                                   pc, memory_order_relaxed))
      return false;
    if (cmp == pc) return true;
  }
  // Pool exhausted, so we can no longer tell a repeat of an
  // already-reported site from a site never seen before.  Off the routed
  // path, keep the historical Die().  On it, report: the dedup is a
  // deduplication of *repeats*, and dropping violations at sites that have
  // never been reported is not something any configured semantic asks for.
  // Erring towards an extra report is recoverable; silently ceasing to
  // check after the 25th distinct site is not.
  if (routed)
    return false;
  Die();
}

void ReportGenericError(uptr pc, uptr bp, uptr sp, uptr addr, bool is_write,
                        uptr access_size, u32 exp, bool fatal) {
  if (__asan_test_only_reported_buggy_pointer) {
    *__asan_test_only_reported_buggy_pointer = addr;
    return;
  }
  if (!fatal && SuppressErrorReport(pc)) return;
  ENABLE_FRAME_POINTER;

  // Optimization experiments.
  // The experiments can be used to evaluate potential optimizations that remove
  // instrumentation (assess false negatives). Instead of completely removing
  // some instrumentation, compiler can emit special calls into runtime
  // (e.g. __asan_report_exp_load1 instead of __asan_report_load1) and pass
  // mask of experiments (exp).
  // The reaction to a non-zero value of exp is to be defined.
  (void)exp;

  ScopedInErrorReport in_report(fatal);
  ErrorGeneric error(GetCurrentTidOrInvalid(), pc, bp, sp, addr, is_write,
                     access_size);
  in_report.ReportError(error);
}

}  // namespace __asan

// --------------------------- Interface --------------------- {{{1
using namespace __asan;

void __asan_report_error(uptr pc, uptr bp, uptr sp, uptr addr, int is_write,
                         uptr access_size, u32 exp) {
  ENABLE_FRAME_POINTER;
  bool fatal = flags()->halt_on_error;
  ReportGenericError(pc, bp, sp, addr, is_write, access_size, exp, fatal);
}

void NOINLINE __asan_set_error_report_callback(void (*callback)(const char*)) {
  // P3100 Task 3.2: runtime guardrail.  When contract routing is active the
  // detected-error report is dispatched through the C++ contract-violation
  // handler, not this stock callback; silently registering it would let the
  // program mix stock ASan callbacks with contract routing.  Refuse with a
  // fatal error naming the opt-out.  When routing is off (opt-out set via
  // -fsanitize-noncontract-callbacks, or a non-p3100 program), the descriptor
  // is absent, AsanContractSemantic() reads stock, and behavior is unchanged.
  const unsigned char route = AsanContractSemantic();
  if (route == kAsanContractObserve || route == kAsanContractEnforce ||
      route == kAsanContractQuick) {
    Report(
        "ERROR: AddressSanitizer: stock error-report callbacks are disabled "
        "under contract routing (-fcontracts-p3100); rebuild with "
        "-fsanitize-noncontract-callbacks to use them\n");
    Die();
  }
  Lock l(&error_message_buf_mutex);
  error_report_callback = callback;
}

void __asan_describe_address(uptr addr) {
  // Thread registry must be locked while we're describing an address.
  asanThreadRegistry().Lock();
  PrintAddressDescription(addr, 1, "");
  asanThreadRegistry().Unlock();
}

int __asan_report_present() {
  return ScopedInErrorReport::CurrentError().kind != kErrorKindInvalid;
}

uptr __asan_get_report_pc() {
  if (ScopedInErrorReport::CurrentError().kind == kErrorKindGeneric)
    return ScopedInErrorReport::CurrentError().Generic.pc;
  return 0;
}

uptr __asan_get_report_bp() {
  if (ScopedInErrorReport::CurrentError().kind == kErrorKindGeneric)
    return ScopedInErrorReport::CurrentError().Generic.bp;
  return 0;
}

uptr __asan_get_report_sp() {
  if (ScopedInErrorReport::CurrentError().kind == kErrorKindGeneric)
    return ScopedInErrorReport::CurrentError().Generic.sp;
  return 0;
}

uptr __asan_get_report_address() {
  ErrorDescription &err = ScopedInErrorReport::CurrentError();
  if (err.kind == kErrorKindGeneric)
    return err.Generic.addr_description.Address();
  else if (err.kind == kErrorKindDoubleFree)
    return err.DoubleFree.addr_description.addr;
  return 0;
}

int __asan_get_report_access_type() {
  if (ScopedInErrorReport::CurrentError().kind == kErrorKindGeneric)
    return ScopedInErrorReport::CurrentError().Generic.is_write;
  return 0;
}

uptr __asan_get_report_access_size() {
  if (ScopedInErrorReport::CurrentError().kind == kErrorKindGeneric)
    return ScopedInErrorReport::CurrentError().Generic.access_size;
  return 0;
}

int __asan_get_report_src_address(uptr* out_addr, uptr* out_size) {
  ErrorDescription& err = ScopedInErrorReport::CurrentError();
  if (err.kind == kErrorKindGeneric && !err.Generic.is_write) {
    if (out_addr)
      *out_addr = err.Generic.addr_description.Address();
    if (out_size)
      *out_size = err.Generic.access_size;
    return 1;
  }
  if (err.kind == kErrorKindStringFunctionMemoryRangesOverlap) {
    if (out_addr)
      *out_addr =
          err.StringFunctionMemoryRangesOverlap.addr2_description.Address();
    if (out_size)
      *out_size = err.StringFunctionMemoryRangesOverlap.length2;
    return 1;
  }
  if (err.kind == kErrorKindStringFunctionSizeOverflow &&
      !err.StringFunctionSizeOverflow.is_write) {
    if (out_addr)
      *out_addr = err.StringFunctionSizeOverflow.addr_description.Address();
    if (out_size)
      *out_size = err.StringFunctionSizeOverflow.size;
    return 1;
  }
  if (err.kind == kErrorKindMallocUsableSizeNotOwned) {
    if (out_addr)
      *out_addr = err.MallocUsableSizeNotOwned.addr_description.Address();
    if (out_size)
      *out_size = 0;
    return 1;
  }
  if (err.kind == kErrorKindSanitizerGetAllocatedSizeNotOwned) {
    if (out_addr)
      *out_addr =
          err.SanitizerGetAllocatedSizeNotOwned.addr_description.Address();
    if (out_size)
      *out_size = 0;
    return 1;
  }
  return 0;
}

int __asan_get_report_dest_address(uptr* out_addr, uptr* out_size) {
  ErrorDescription& err = ScopedInErrorReport::CurrentError();
  if (err.kind == kErrorKindGeneric && err.Generic.is_write) {
    if (out_addr)
      *out_addr = err.Generic.addr_description.Address();
    if (out_size)
      *out_size = err.Generic.access_size;
    return 1;
  }
  if (err.kind == kErrorKindStringFunctionMemoryRangesOverlap) {
    if (out_addr)
      *out_addr =
          err.StringFunctionMemoryRangesOverlap.addr1_description.Address();
    if (out_size)
      *out_size = err.StringFunctionMemoryRangesOverlap.length1;
    return 1;
  }
  if (err.kind == kErrorKindStringFunctionSizeOverflow &&
      err.StringFunctionSizeOverflow.is_write) {
    if (out_addr)
      *out_addr = err.StringFunctionSizeOverflow.addr_description.Address();
    if (out_size)
      *out_size = err.StringFunctionSizeOverflow.size;
    return 1;
  }
  return 0;
}

int __asan_get_report_dealloc_address(uptr* out_addr, uptr* out_size) {
  ErrorDescription& err = ScopedInErrorReport::CurrentError();
  if (err.kind == kErrorKindDoubleFree) {
    if (out_addr)
      *out_addr = err.DoubleFree.addr_description.addr;
    if (out_size)
      *out_size = 0;
    return 1;
  }
  if (err.kind == kErrorKindNewDeleteTypeMismatch) {
    if (out_addr)
      *out_addr = err.NewDeleteTypeMismatch.addr_description.addr;
    if (out_size)
      *out_size = err.NewDeleteTypeMismatch.delete_size;
    return 1;
  }
  if (err.kind == kErrorKindFreeNotMalloced) {
    if (out_addr)
      *out_addr = err.FreeNotMalloced.addr_description.Address();
    if (out_size)
      *out_size = 0;
    return 1;
  }
  if (err.kind == kErrorKindAllocTypeMismatch) {
    if (out_addr)
      *out_addr = err.AllocTypeMismatch.addr_description.Address();
    if (out_size)
      *out_size = 0;
    return 1;
  }
  return 0;
}

int __asan_get_report_first_address(uptr* out_addr, uptr* out_size) {
  ErrorDescription& err = ScopedInErrorReport::CurrentError();
  if (err.kind == kErrorKindInvalidPointerPair) {
    if (out_addr)
      *out_addr = err.InvalidPointerPair.addr1_description.Address();
    if (out_size)
      *out_size = 0;
    return 1;
  }
  if (err.kind == kErrorKindODRViolation) {
    if (out_addr)
      *out_addr = err.ODRViolation.global1.beg;
    if (out_size)
      *out_size = 0;
    return 1;
  }
  return 0;
}

int __asan_get_report_second_address(uptr* out_addr, uptr* out_size) {
  ErrorDescription& err = ScopedInErrorReport::CurrentError();
  if (err.kind == kErrorKindInvalidPointerPair) {
    if (out_addr)
      *out_addr = err.InvalidPointerPair.addr2_description.Address();
    if (out_size)
      *out_size = 0;
    return 1;
  }
  if (err.kind == kErrorKindODRViolation) {
    if (out_addr)
      *out_addr = err.ODRViolation.global2.beg;
    if (out_size)
      *out_size = 0;
    return 1;
  }
  return 0;
}

const char *__asan_get_report_description() {
  if (ScopedInErrorReport::CurrentError().kind == kErrorKindGeneric)
    return ScopedInErrorReport::CurrentError().Generic.bug_descr;
  return ScopedInErrorReport::CurrentError().Base.scariness.GetDescription();
}

extern "C" {
SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_ptr_sub(void *a, void *b) {
  CheckForInvalidPointerPair(a, b, kAsanCheckPointerSubtract);
}
SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_ptr_cmp(void *a, void *b) {
  CheckForInvalidPointerPair(a, b, kAsanCheckPointerCompare);
}
} // extern "C"

// Provide default implementation of __asan_on_error that does nothing
// and may be overridden by user.
SANITIZER_INTERFACE_WEAK_DEF(void, __asan_on_error, void) {}
