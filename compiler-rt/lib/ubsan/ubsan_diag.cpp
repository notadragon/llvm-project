//===-- ubsan_diag.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Diagnostic reporting for the UBSan runtime.
//
//===----------------------------------------------------------------------===//

#include "ubsan_platform.h"
#if CAN_SANITIZE_UB
#include "ubsan_diag.h"
#include "ubsan_flags.h"
#include "ubsan_init.h"
#include "ubsan_monitor.h"

#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_placement_new.h"
#include "sanitizer_common/sanitizer_report_decorator.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "sanitizer_common/sanitizer_stacktrace_printer.h"
#include "sanitizer_common/sanitizer_suppressions.h"
#include "sanitizer_common/sanitizer_symbolizer.h"

#include <stdio.h>

using namespace __ubsan;

// Can be overriden in frontend.
SANITIZER_INTERFACE_WEAK_DEF(const char *, __ubsan_default_suppressions, void) {
  return "";
}

// ---------------------------------------------------------------------------
// P3100 contract routing (mirror of the ASan path in asan/asan_report.cpp).
//
// Under -fcontracts-p3100 the compiler emits a per-TU weak table,
// __ubsan_contract_semantic[], one wire byte per routed -fsanitize= runtime
// check.  When a routed check's report reaches the runtime, instead of printing
// the stock "runtime error: ..." diagnostic and consulting UBSAN_OPTIONS, we
// build a contract_violation (kind = implicit) and call the C++ contract-
// violation handler, which then owns ALL output.  The handler may fetch the
// rich UBSan text lazily via contract_violation::report().
//
// Throwing handlers: the handler is invoked from inside the implicitly-noexcept
// ScopedReport destructor below, so a throwing handler can never propagate --
// it would hit "exception escaping a noexcept function" and std::terminate()
// several frames below any user catch/RAII.  That is why the compiler only ever
// routes the NON-throwing semantics here: observe/enforce on the wire are
// really the D4298 noexcept_observe/noexcept_enforce paths (gated on
// -fcontracts-p4298), and quick_enforce terminates without ever entering the
// handler.  Same rule and wire encoding as ASan (asan_report.cpp).
// ---------------------------------------------------------------------------
namespace {
// Wire encoding, identical to __asan_contract_semantic (asan_report.cpp) and
// the compiler side (gcc/cp/decl2.cc emit_ubsan_contract_semantic_descriptor).
enum {
  kUbsanContractStock = 0,   // routing off: stock behavior
  kUbsanContractObserve = 1, // noexcept_observe: call handler, then continue
  kUbsanContractEnforce = 2, // noexcept_enforce: call handler, then terminate
  kUbsanContractQuick = 3,   // quick_enforce: terminate WITHOUT the handler
};

// Routed-check ids indexing the per-TU weak table __ubsan_contract_semantic[].
// KEEP IN SYNC with gcc/cp/decl2.cc (the RUC_* / RUC_COUNT mirror).  Each entry
// is one -fsanitize= runtime check routed to the contract-violation handler;
// add a check by appending an id here and a case to ErrorTypeToRoutedId below
// (and the matching entry in the compiler-side descriptor emitter).
// RUC_FUNCTION is reserved for cross-compiler id alignment with Clang, which
// routes -fsanitize=function; GCC never emits that check, so on GCC the slot is
// always stock (but the ErrorType is still folded below, harmlessly).
enum {
  RUC_VPTR = 0,    // -fsanitize=vptr (ErrorType::DynamicTypeMismatch)
  RUC_FUNCTION,    // -fsanitize=function (ErrorType::FunctionTypeMismatch)
  RUC_ALIGNMENT,   // -fsanitize=alignment
  RUC_OBJECT_SIZE, // -fsanitize=object-size
  RUC_NONNULL_ATTRIBUTE,         // -fsanitize=nonnull-attribute
  RUC_RETURNS_NONNULL_ATTRIBUTE, // -fsanitize=returns-nonnull-attribute
  RUC_POINTER_OVERFLOW,          // -fsanitize=pointer-overflow
  RUC_NULL,                      // -fsanitize=null
  RUC_SHIFT_BASE,                // -fsanitize=shift-base
  RUC_SHIFT_EXPONENT,            // -fsanitize=shift-exponent
  RUC_INTEGER_DIVIDE_BY_ZERO,    // -fsanitize=integer-divide-by-zero
  RUC_SIGNED_INTEGER_OVERFLOW,   // -fsanitize=signed-integer-overflow
  RUC_BOOL,                      // -fsanitize=bool
  RUC_ENUM,                      // -fsanitize=enum
  RUC_FLOAT_CAST_OVERFLOW,       // -fsanitize=float-cast-overflow
  RUC_BOUNDS,                    // -fsanitize=bounds / array-bounds
  RUC_RETURN,                    // -fsanitize=return
  RUC_UNREACHABLE,               // -fsanitize=unreachable
  RUC_VLA_BOUND,                 // -fsanitize=vla-bound
  RUC_BUILTIN,                   // -fsanitize=builtin
  RUC_FLOAT_DIVIDE_BY_ZERO,      // -fsanitize=float-divide-by-zero
  // Clang-only checks (GCC has no -fsanitize= bit for them); reserved for
  // cross-compiler id alignment.  Their ErrorTypes are still folded below, but
  // on GCC their wire slots stay stock since the descriptor never sets them.
  RUC_UNSIGNED_INTEGER_OVERFLOW, // -fsanitize=unsigned-integer-overflow
  RUC_IMPLICIT_CONVERSION,       // -fsanitize=implicit-* conversion checks
  RUC_LOCAL_BOUNDS,              // -fsanitize=local-bounds
  RUC_OBJC_CAST,                 // -fsanitize=objc-cast
  RUC_COUNT
};
const int RUC_INVALID = -1;

// Fold an ErrorType to its routed-check id (RUC_INVALID = not routed).  Several
// ErrorTypes may map to one -fsanitize= check; list only the routed ones.
int ErrorTypeToRoutedId(ErrorType Type) {
  switch (Type) {
  case ErrorType::DynamicTypeMismatch:
    return RUC_VPTR;
  case ErrorType::FunctionTypeMismatch:
    return RUC_FUNCTION;
  case ErrorType::MisalignedPointerUse:
  case ErrorType::AlignmentAssumption:
    return RUC_ALIGNMENT;
  case ErrorType::InsufficientObjectSize:
    return RUC_OBJECT_SIZE;
  case ErrorType::InvalidNullArgument:
  case ErrorType::InvalidNullArgumentWithNullability:
    return RUC_NONNULL_ATTRIBUTE;
  case ErrorType::InvalidNullReturn:
  case ErrorType::InvalidNullReturnWithNullability:
    return RUC_RETURNS_NONNULL_ATTRIBUTE;
  case ErrorType::NullptrWithOffset:
  case ErrorType::NullptrWithNonZeroOffset:
  case ErrorType::NullptrAfterNonZeroOffset:
  case ErrorType::PointerOverflow:
    return RUC_POINTER_OVERFLOW;
  case ErrorType::NullPointerUse:
  case ErrorType::NullPointerUseWithNullability:
    return RUC_NULL;
  case ErrorType::InvalidShiftBase:
    return RUC_SHIFT_BASE;
  case ErrorType::InvalidShiftExponent:
    return RUC_SHIFT_EXPONENT;
  case ErrorType::IntegerDivideByZero:
    return RUC_INTEGER_DIVIDE_BY_ZERO;
  case ErrorType::SignedIntegerOverflow:
    return RUC_SIGNED_INTEGER_OVERFLOW;
  case ErrorType::InvalidBoolLoad:
    return RUC_BOOL;
  case ErrorType::InvalidEnumLoad:
    return RUC_ENUM;
  case ErrorType::FloatCastOverflow:
    return RUC_FLOAT_CAST_OVERFLOW;
  case ErrorType::OutOfBoundsIndex:
    return RUC_BOUNDS;
  case ErrorType::MissingReturn:
    return RUC_RETURN;
  case ErrorType::UnreachableCall:
    return RUC_UNREACHABLE;
  case ErrorType::NonPositiveVLAIndex:
    return RUC_VLA_BOUND;
  case ErrorType::InvalidBuiltin:
    return RUC_BUILTIN;
  case ErrorType::FloatDivideByZero:
    return RUC_FLOAT_DIVIDE_BY_ZERO;
  case ErrorType::UnsignedIntegerOverflow:
    return RUC_UNSIGNED_INTEGER_OVERFLOW;
  case ErrorType::ImplicitUnsignedIntegerTruncation:
  case ErrorType::ImplicitSignedIntegerTruncation:
  case ErrorType::ImplicitIntegerSignChange:
  case ErrorType::ImplicitSignedIntegerTruncationOrSignChange:
    return RUC_IMPLICIT_CONVERSION;
  case ErrorType::LocalOutOfBounds:
    return RUC_LOCAL_BOUNDS;
  case ErrorType::InvalidObjCCast:
    return RUC_OBJC_CAST;
  default:
    return RUC_INVALID;
  }
}
} // namespace

// Per-TU weak table: one wire byte per routed check.  Absent (all reads stock)
// in a program not built with contract routing.  Declared weak so a non-p3100
// program links and reads stock, exactly like ASan's scalar descriptor.
extern "C" SANITIZER_WEAK_ATTRIBUTE unsigned char __ubsan_contract_semantic[];

// Return the conveyed semantic for routed id RUC (0 = stock when the descriptor
// is absent or RUC is not a routed check).
static unsigned char UbsanContractSemantic(int ruc) {
  if (ruc < 0 || ruc >= RUC_COUNT)
    return kUbsanContractStock;
  if (&__ubsan_contract_semantic == nullptr)
    return kUbsanContractStock;
  return __ubsan_contract_semantic[ruc];
}

// The lazy report populator ABI struct (mirror of
// __cxa_contract_report_populator in libstdc++ bits/contracts_abi.h and the
// AsanContractReportPopulator in asan_report.cpp).  Layout must be { const
// char* (*)(const void*), const void* }.
struct UbsanContractReportPopulator {
  const char *(*populate)(const void *ctx);
  const void *ctx;
};
struct UbsanContractReportCtx {
  const char *rendered; // the already-captured NUL-terminated report text
};

// The contract-violation report leg, provided by the C++ runtime (libstdc++).
// Declared weak: absent when the contracts runtime is not linked, in which case
// we fall back to emitting the captured diagnostic (see ~ScopedReport).  Builds
// an implicit contract_violation and invokes the handler; always returns
// (termination for enforce is performed here).  The final argument is the lazy
// report populator (CXA_FIELD_REPORT): the handler's
// contract_violation::report() invokes populate(ctx) on demand.  Same
// symbol/signature as the ASan path.
extern "C" SANITIZER_WEAK_ATTRIBUTE void
__cxa_contract_violation_sanitizer(const char *comment, const char *file,
                                   unsigned line, unsigned char semantic,
                                   const UbsanContractReportPopulator *report);

// Live capture of the rendered UBSan text on the routed path.  Unlike ASan
// (whose error object is replayable, so it renders on demand), a UBSan Diag
// prints eagerly and leaves nothing to re-render; so we capture the text as the
// Diags run (Diag::~Diag appends here when g_contract_capturing) and suppress
// stderr.  All reporting is serialized by ScopedReport's report lock, so a
// single static buffer + flag is safe (mirrors asan_report.cpp's static
// rendered_report).  The populator hands this back to report() on demand.
static const uptr kUbsanContractCaptureSize = 8192;
static char g_contract_capture[kUbsanContractCaptureSize];
static uptr g_contract_capture_len;
static bool g_contract_capturing;

static void ContractCaptureBegin() {
  g_contract_capturing = true;
  g_contract_capture_len = 0;
  g_contract_capture[0] = '\0';
}
static void ContractCaptureEnd() { g_contract_capturing = false; }
static void ContractCaptureAppend(const char *s) {
  if (!s)
    return;
  uptr n = internal_strlen(s);
  uptr room = kUbsanContractCaptureSize - 1 - g_contract_capture_len;
  if (n > room)
    n = room;
  internal_memcpy(g_contract_capture + g_contract_capture_len, s, n);
  g_contract_capture_len += n;
  g_contract_capture[g_contract_capture_len] = '\0';
}

// RF5 lazy populator: the text was already captured live (cheap string append),
// so this just returns it.  Only ever runs if the handler calls report().
static const char *ubsan_contract_report_populate(const void *ctx_v) {
  const UbsanContractReportCtx *ctx =
      static_cast<const UbsanContractReportCtx *>(ctx_v);
  if (!ctx)
    return nullptr;
  return ctx->rendered ? ctx->rendered : "";
}

// UBSan is combined with runtimes that already provide this functionality
// (e.g., ASan) as well as runtimes that lack it (e.g., scudo). Tried to use
// weak linkage to resolve this issue which is not portable and breaks on
// Windows.
// TODO(yln): This is a temporary workaround. GetStackTrace functions will be
// removed in the future.
void ubsan_GetStackTrace(BufferedStackTrace *stack, uptr max_depth, uptr pc,
                         uptr bp, void *context, bool request_fast) {
  uptr top = 0;
  uptr bottom = 0;
  GetThreadStackTopAndBottom(false, &top, &bottom);
  bool fast = StackTrace::WillUseFastUnwind(request_fast);
  stack->Unwind(max_depth, pc, bp, context, top, bottom, fast);
}

static void MaybePrintStackTrace(uptr pc, uptr bp) {
  // We assume that flags are already parsed, as UBSan runtime
  // will definitely be called when we print the first diagnostics message.
  if (!flags()->print_stacktrace)
    return;

  UNINITIALIZED BufferedStackTrace stack;
  ubsan_GetStackTrace(&stack, kStackTraceMax, pc, bp, nullptr,
                common_flags()->fast_unwind_on_fatal);
  stack.Print();
}

static const char *ConvertTypeToString(ErrorType Type) {
  switch (Type) {
#define UBSAN_CHECK(Name, SummaryKind, FSanitizeFlagName)                      \
  case ErrorType::Name:                                                        \
    return SummaryKind;
#include "ubsan_checks.inc"
#undef UBSAN_CHECK
  }
  UNREACHABLE("unknown ErrorType!");
}

static const char *ConvertTypeToFlagName(ErrorType Type) {
  switch (Type) {
#define UBSAN_CHECK(Name, SummaryKind, FSanitizeFlagName)                      \
  case ErrorType::Name:                                                        \
    return FSanitizeFlagName;
#include "ubsan_checks.inc"
#undef UBSAN_CHECK
  }
  UNREACHABLE("unknown ErrorType!");
}

static void MaybeReportErrorSummary(Location Loc, ErrorType Type) {
  if (!common_flags()->print_summary)
    return;
  if (!flags()->report_error_type)
    Type = ErrorType::GenericUB;
  const char *ErrorKind = ConvertTypeToString(Type);
  if (Loc.isSourceLocation()) {
    SourceLocation SLoc = Loc.getSourceLocation();
    if (!SLoc.isInvalid()) {
      AddressInfo AI;
      AI.file = internal_strdup(SLoc.getFilename());
      AI.line = SLoc.getLine();
      AI.column = SLoc.getColumn();
      AI.function = nullptr;
      ReportErrorSummary(ErrorKind, AI, GetSanititizerToolName());
      AI.Clear();
      return;
    }
  } else if (Loc.isSymbolizedStack()) {
    const AddressInfo &AI = Loc.getSymbolizedStack()->info;
    ReportErrorSummary(ErrorKind, AI, GetSanititizerToolName());
    return;
  }
  ReportErrorSummary(ErrorKind, GetSanititizerToolName());
}

namespace {
class Decorator : public SanitizerCommonDecorator {
 public:
  Decorator() : SanitizerCommonDecorator() {}
  const char *Highlight() const { return Green(); }
  const char *Note() const { return Black(); }
};
}

SymbolizedStack *__ubsan::getSymbolizedLocation(uptr PC) {
  InitAsStandaloneIfNecessary();
  return Symbolizer::GetOrInit()->SymbolizePC(PC);
}

Diag &Diag::operator<<(const TypeDescriptor &V) {
  return AddArg(V.getTypeName());
}

Diag &Diag::operator<<(const Value &V) {
  if (V.getType().isSignedIntegerTy())
    AddArg(V.getSIntValue());
  else if (V.getType().isUnsignedIntegerTy())
    AddArg(V.getUIntValue());
  else if (V.getType().isFloatTy())
    AddArg(V.getFloatValue());
  else
    AddArg("<unknown>");
  return *this;
}

/// Hexadecimal printing for numbers too large for Printf to handle directly.
static void RenderHex(InternalScopedString *Buffer, UIntMax Val) {
#if HAVE_INT128_T
  Buffer->AppendF("0x%08x%08x%08x%08x", (unsigned int)(Val >> 96),
                  (unsigned int)(Val >> 64), (unsigned int)(Val >> 32),
                  (unsigned int)(Val));
#else
  UNREACHABLE("long long smaller than 64 bits?");
#endif
}

static void RenderLocation(InternalScopedString *Buffer, Location Loc) {
  switch (Loc.getKind()) {
  case Location::LK_Source: {
    SourceLocation SLoc = Loc.getSourceLocation();
    if (SLoc.isInvalid())
      Buffer->AppendF("<unknown>");
    else
      StackTracePrinter::GetOrInit()->RenderSourceLocation(
          Buffer, SLoc.getFilename(), SLoc.getLine(), SLoc.getColumn(),
          common_flags()->symbolize_vs_style,
          common_flags()->strip_path_prefix);
    return;
  }
  case Location::LK_Memory:
    Buffer->AppendF("%p", reinterpret_cast<void *>(Loc.getMemoryLocation()));
    return;
  case Location::LK_Symbolized: {
    const AddressInfo &Info = Loc.getSymbolizedStack()->info;
    if (Info.file)
      StackTracePrinter::GetOrInit()->RenderSourceLocation(
          Buffer, Info.file, Info.line, Info.column,
          common_flags()->symbolize_vs_style,
          common_flags()->strip_path_prefix);
    else if (Info.module)
      StackTracePrinter::GetOrInit()->RenderModuleLocation(
          Buffer, Info.module, Info.module_offset, Info.module_arch,
          common_flags()->strip_path_prefix);
    else
      Buffer->AppendF("%p", reinterpret_cast<void *>(Info.address));
    return;
  }
  case Location::LK_Null:
    Buffer->AppendF("<unknown>");
    return;
  }
}

static void RenderText(InternalScopedString *Buffer, const char *Message,
                       const Diag::Arg *Args) {
  for (const char *Msg = Message; *Msg; ++Msg) {
    if (*Msg != '%') {
      Buffer->AppendF("%c", *Msg);
      continue;
    }
    const Diag::Arg &A = Args[*++Msg - '0'];
    switch (A.Kind) {
    case Diag::AK_String:
      Buffer->AppendF("%s", A.String);
      break;
    case Diag::AK_TypeName: {
      if (SANITIZER_WINDOWS)
        // The Windows implementation demangles names early.
        Buffer->AppendF("'%s'", A.String);
      else
        Buffer->AppendF("'%s'", Symbolizer::GetOrInit()->Demangle(A.String));
      break;
    }
    case Diag::AK_SInt:
      // 'long long' is guaranteed to be at least 64 bits wide.
      if (A.SInt >= INT64_MIN && A.SInt <= INT64_MAX)
        Buffer->AppendF("%lld", (long long)A.SInt);
      else
        RenderHex(Buffer, A.SInt);
      break;
    case Diag::AK_UInt:
      if (A.UInt <= UINT64_MAX)
        Buffer->AppendF("%llu", (unsigned long long)A.UInt);
      else
        RenderHex(Buffer, A.UInt);
      break;
    case Diag::AK_Float: {
      // FIXME: Support floating-point formatting in sanitizer_common's
      //        printf, and stop using snprintf here.
      char FloatBuffer[32];
#if SANITIZER_WINDOWS
      // On MSVC platforms, long doubles are equal to regular doubles.
      // In MinGW environments on x86, long doubles are 80 bit, but here,
      // we're calling an MS CRT provided printf function which considers
      // long doubles to be 64 bit. Just cast the float value to a regular
      // double to avoid the potential ambiguity in MinGW mode.
      sprintf_s(FloatBuffer, sizeof(FloatBuffer), "%g", (double)A.Float);
#else
      snprintf(FloatBuffer, sizeof(FloatBuffer), "%Lg", (long double)A.Float);
#endif
      Buffer->Append(FloatBuffer);
      break;
    }
    case Diag::AK_Pointer:
      Buffer->AppendF("%p", A.Pointer);
      break;
    }
  }
}

/// Find the earliest-starting range in Ranges which ends after Loc.
static Range *upperBound(MemoryLocation Loc, Range *Ranges,
                         unsigned NumRanges) {
  Range *Best = 0;
  for (unsigned I = 0; I != NumRanges; ++I)
    if (Ranges[I].getEnd().getMemoryLocation() > Loc &&
        (!Best ||
         Best->getStart().getMemoryLocation() >
         Ranges[I].getStart().getMemoryLocation()))
      Best = &Ranges[I];
  return Best;
}

static inline uptr subtractNoOverflow(uptr LHS, uptr RHS) {
  return (LHS < RHS) ? 0 : LHS - RHS;
}

static inline uptr addNoOverflow(uptr LHS, uptr RHS) {
  const uptr Limit = (uptr)-1;
  return (LHS > Limit - RHS) ? Limit : LHS + RHS;
}

/// Render a snippet of the address space near a location.
static void PrintMemorySnippet(const Decorator &Decor, MemoryLocation Loc,
                               Range *Ranges, unsigned NumRanges,
                               const Diag::Arg *Args) {
  // Show at least the 8 bytes surrounding Loc.
  const unsigned MinBytesNearLoc = 4;
  MemoryLocation Min = subtractNoOverflow(Loc, MinBytesNearLoc);
  MemoryLocation Max = addNoOverflow(Loc, MinBytesNearLoc);
  MemoryLocation OrigMin = Min;
  for (unsigned I = 0; I < NumRanges; ++I) {
    Min = __sanitizer::Min(Ranges[I].getStart().getMemoryLocation(), Min);
    Max = __sanitizer::Max(Ranges[I].getEnd().getMemoryLocation(), Max);
  }

  // If we have too many interesting bytes, prefer to show bytes after Loc.
  const unsigned BytesToShow = 32;
  if (Max - Min > BytesToShow)
    Min = __sanitizer::Min(Max - BytesToShow, OrigMin);
  Max = addNoOverflow(Min, BytesToShow);

  if (!IsAccessibleMemoryRange(Min, Max - Min)) {
    Printf("<memory cannot be printed>\n");
    return;
  }

  // Emit data.
  InternalScopedString Buffer;
  for (uptr P = Min; P != Max; ++P) {
    unsigned char C = *reinterpret_cast<const unsigned char*>(P);
    Buffer.AppendF("%s%02x", (P % 8 == 0) ? "  " : " ", C);
  }
  Buffer.AppendF("\n");

  // Emit highlights.
  Buffer.Append(Decor.Highlight());
  Range *InRange = upperBound(Min, Ranges, NumRanges);
  for (uptr P = Min; P != Max; ++P) {
    char Pad = ' ', Byte = ' ';
    if (InRange && InRange->getEnd().getMemoryLocation() == P)
      InRange = upperBound(P, Ranges, NumRanges);
    if (!InRange && P > Loc)
      break;
    if (InRange && InRange->getStart().getMemoryLocation() < P)
      Pad = '~';
    if (InRange && InRange->getStart().getMemoryLocation() <= P)
      Byte = '~';
    if (P % 8 == 0)
      Buffer.AppendF("%c", Pad);
    Buffer.AppendF("%c", Pad);
    Buffer.AppendF("%c", P == Loc ? '^' : Byte);
    Buffer.AppendF("%c", Byte);
  }
  Buffer.AppendF("%s\n", Decor.Default());

  // Go over the line again, and print names for the ranges.
  InRange = 0;
  unsigned Spaces = 0;
  for (uptr P = Min; P != Max; ++P) {
    if (!InRange || InRange->getEnd().getMemoryLocation() == P)
      InRange = upperBound(P, Ranges, NumRanges);
    if (!InRange)
      break;

    Spaces += (P % 8) == 0 ? 2 : 1;

    if (InRange && InRange->getStart().getMemoryLocation() == P) {
      while (Spaces--)
        Buffer.AppendF(" ");
      RenderText(&Buffer, InRange->getText(), Args);
      Buffer.AppendF("\n");
      // FIXME: We only support naming one range for now!
      break;
    }

    Spaces += 2;
  }

  Printf("%s", Buffer.data());
  // FIXME: Print names for anything we can identify within the line:
  //
  //  * If we can identify the memory itself as belonging to a particular
  //    global, stack variable, or dynamic allocation, then do so.
  //
  //  * If we have a pointer-size, pointer-aligned range highlighted,
  //    determine whether the value of that range is a pointer to an
  //    entity which we can name, and if so, print that name.
  //
  // This needs an external symbolizer, or (preferably) ASan instrumentation.
}

Diag::~Diag() {
  // All diagnostics should be printed under report mutex.
  ScopedReport::CheckLocked();
  Decorator Decor;
  InternalScopedString Buffer;

  // Prepare a report that a monitor process can inspect.
  if (Level == DL_Error) {
    RenderText(&Buffer, Message, Args);
    UndefinedBehaviorReport UBR{ConvertTypeToString(ET), Loc, Buffer};
    Buffer.clear();
  }

  Buffer.Append(Decor.Bold());
  RenderLocation(&Buffer, Loc);
  Buffer.AppendF(":");

  switch (Level) {
  case DL_Error:
    Buffer.AppendF("%s runtime error: %s%s", Decor.Warning(), Decor.Default(),
                   Decor.Bold());
    break;

  case DL_Note:
    Buffer.AppendF("%s note: %s", Decor.Note(), Decor.Default());
    break;
  }

  RenderText(&Buffer, Message, Args);

  Buffer.AppendF("%s\n", Decor.Default());

  // P3100: on the contract-routed path capture the rendered line for the
  // handler's contract_violation::report() and emit NOTHING to stderr -- the
  // contract-violation handler owns all output.  (ScopedReport enabled capture
  // in its constructor, which runs before any Diag here.)  The memory snippet
  // is likewise suppressed while routing.  Stock behavior is byte-for-byte
  // unchanged when routing is off.
  if (g_contract_capturing) {
    ContractCaptureAppend(Buffer.data());
    return;
  }

  Printf("%s", Buffer.data());

  if (Loc.isMemoryLocation())
    PrintMemorySnippet(Decor, Loc.getMemoryLocation(), Ranges, NumRanges, Args);
}

ScopedReport::Initializer::Initializer() { InitAsStandaloneIfNecessary(); }

ScopedReport::ScopedReport(ReportOptions Opts, Location SummaryLoc,
                           ErrorType Type)
    : Opts(Opts), SummaryLoc(SummaryLoc), Type(Type),
      contract_wire_(UbsanContractSemantic(ErrorTypeToRoutedId(Type))) {
  // P3100: begin capturing the rendered diagnostic (and suppressing stderr) for
  // any routed report, so the destructor can hand it to the contract-violation
  // handler.  The report lock is already held (member init above), which
  // serializes access to the capture statics.
  if (contract_wire_ != kUbsanContractStock)
    ContractCaptureBegin();
}

ScopedReport::~ScopedReport() {
  // P3100 contract routing: when this report's check is routed, the configured
  // semantic ALONE decides behavior (no dependence on UBSAN_OPTIONS/
  // halt_on_error) -- mirror of ASan's ScopedInErrorReport dtor.  The Diags
  // have already rendered into g_contract_capture with stderr suppressed; here
  // we dispatch that to the handler (observe/enforce) or terminate silently
  // (quick).
  const unsigned char route = contract_wire_;
  if (route != kUbsanContractStock) {
    ContractCaptureEnd();
    const bool handler_linked =
        (&__cxa_contract_violation_sanitizer != nullptr);

    if (route == kUbsanContractQuick) {
      // quick_enforce: terminate silently -- no handler, no report, no output.
      Die();
    }

    if ((route == kUbsanContractObserve || route == kUbsanContractEnforce) &&
        handler_linked) {
      // UBSan knows the concrete error and its source location -- pass both so
      // the contract_violation carries a real file:line (better fidelity than
      // ASan, which supplies none).  The rich text is served lazily via
      // report().
      const char *comment = ConvertTypeToFlagName(Type);
      const char *file = "";
      unsigned line = 0;
      if (SummaryLoc.isSourceLocation()) {
        SourceLocation SLoc = SummaryLoc.getSourceLocation();
        if (!SLoc.isInvalid()) {
          file = SLoc.getFilename();
          line = SLoc.getLine();
        }
      }
      UbsanContractReportCtx ctx = {/*rendered=*/g_contract_capture};
      UbsanContractReportPopulator pop = {&ubsan_contract_report_populate,
                                          &ctx};
      __cxa_contract_violation_sanitizer(comment, file, line, route, &pop);
      if (route == kUbsanContractObserve)
        return; // noexcept_observe: continue past the violation.
      // noexcept_enforce: terminate.  The handler owns all output on this path,
      // so emit no extra text.
      Die();
    }

    // Routed, but the C++ contracts runtime is not linked (defensive: should
    // not happen for a -fcontracts-p3100 program).  Don't lose the diagnostic:
    // emit the captured text, then fall through to the stock termination
    // policy.
    if (g_contract_capture[0])
      Printf("%s", g_contract_capture);
  }

  MaybePrintStackTrace(Opts.pc, Opts.bp);
  MaybeReportErrorSummary(SummaryLoc, Type);

  if (common_flags()->print_module_map >= 2)
    DumpProcessMap();

  if (flags()->halt_on_error)
    Die();
}

alignas(64) static char suppression_placeholder[sizeof(SuppressionContext)];
static SuppressionContext *suppression_ctx = nullptr;
static const char kVptrCheck[] = "vptr_check";
static const char *kSuppressionTypes[] = {
#define UBSAN_CHECK(Name, SummaryKind, FSanitizeFlagName) FSanitizeFlagName,
#include "ubsan_checks.inc"
#undef UBSAN_CHECK
    kVptrCheck,
};

void __ubsan::InitializeSuppressions() {
  CHECK_EQ(nullptr, suppression_ctx);
  suppression_ctx = new (suppression_placeholder)
      SuppressionContext(kSuppressionTypes, ARRAY_SIZE(kSuppressionTypes));
  suppression_ctx->ParseFromFile(flags()->suppressions);
  suppression_ctx->Parse(__ubsan_default_suppressions());
}

bool __ubsan::IsVptrCheckSuppressed(const char *TypeName) {
  InitAsStandaloneIfNecessary();
  CHECK(suppression_ctx);
  Suppression *s;
  return suppression_ctx->Match(TypeName, kVptrCheck, &s);
}

bool __ubsan::IsPCSuppressed(ErrorType ET, uptr PC, const char *Filename) {
  InitAsStandaloneIfNecessary();
  CHECK(suppression_ctx);
  // PC is the return address from the UBSan handler call. Symbolize the
  // instruction that caused the call.
  PC = StackTrace::GetPreviousInstructionPc(PC);
  const char *SuppType = ConvertTypeToFlagName(ET);
  // Fast path: don't symbolize PC if there is no suppressions for given UB
  // type.
  if (!suppression_ctx->HasSuppressionType(SuppType))
    return false;
  Suppression *s = nullptr;
  // Suppress by file name known to runtime.
  if (Filename != nullptr && suppression_ctx->Match(Filename, SuppType, &s))
    return true;
  // Suppress by module name.
  if (const char *Module = Symbolizer::GetOrInit()->GetModuleNameForPc(PC)) {
    if (suppression_ctx->Match(Module, SuppType, &s))
      return true;
  }
  // Suppress by function or source file name from debug info.
  // The first frame is the innermost logical inline frame, if inline debug
  // information is available. Do not search the rest of the chain: a
  // suppression for an inline wrapper must not suppress an inlined callee.
  SymbolizedStackHolder Stack(Symbolizer::GetOrInit()->SymbolizePC(PC));
  const AddressInfo &AI = Stack.get()->info;
  return suppression_ctx->Match(AI.function, SuppType, &s) ||
         suppression_ctx->Match(AI.file, SuppType, &s);
}

#endif  // CAN_SANITIZE_UB
