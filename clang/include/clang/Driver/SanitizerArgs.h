//===--- SanitizerArgs.h - Arguments for sanitizer tools  -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_DRIVER_SANITIZERARGS_H
#define LLVM_CLANG_DRIVER_SANITIZERARGS_H

#include "clang/Basic/ContractOptions.h"
#include "clang/Basic/OffloadArch.h"
#include "clang/Basic/Sanitizers.h"
#include "clang/Driver/Action.h"
#include "clang/Driver/Types.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Transforms/Instrumentation/AddressSanitizerOptions.h"
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace clang {
namespace driver {

class ToolChain;
class Driver;

class SanitizerArgs {
  SanitizerSet Sanitizers;
  SanitizerSet RecoverableSanitizers;
  SanitizerSet TrapSanitizers;

  /// P3100: contract evaluation semantics explicitly requested per sanitizer
  /// check via -fsanitize-semantic=.  Each entry's mask is a single sanitizer
  /// bit; a check with no explicit request has no entry (its semantic is
  /// derived, see resolvedSanitizerSemantic).  A validated request is never
  /// out of the check's allowed set (see the P3100 Task 1.3/4.1 checks), so
  /// this store never holds an illegal semantic for any bit.
  std::vector<std::pair<SanitizerMask, ContractEvaluationSemantic>>
      ExplicitSemantics;
  /// Whether -fsanitize-semantic-print was requested (debug testing seam).
  bool SanitizeSemanticPrint = false;
  /// P3100 Task 3.1: whether -fsanitize-noncontract-callbacks was requested --
  /// the global opt-out that restores the sanitizer's stock reporting and
  /// callback behavior instead of contract routing.
  bool SanitizeNoncontractCallbacks = false;
  /// Whether -fcontracts-p4298 is in effect (gates the noexcept semantics for
  /// a routed sanitizer check; see the P3100 Task 4.1 model).
  bool ContractsP4298 = false;
  /// Whether -fcontracts-p3100 is in effect (routing to the contract-violation
  /// handler is only active under it).
  bool ContractsP3100 = false;
  SanitizerSet MergeHandlers;
  SanitizerMaskCutoffs SkipHotCutoffs;
  SanitizerSet AnnotateDebugInfo;
  SanitizerSet SuppressUBSanFeature;

  std::vector<std::string> UserIgnorelistFiles;
  std::vector<std::string> SystemIgnorelistFiles;
  std::vector<std::string> CoverageAllowlistFiles;
  std::vector<std::string> CoverageIgnorelistFiles;
  std::vector<std::string> BinaryMetadataIgnorelistFiles;
  int CoverageFeatures = 0;
  int CoverageStackDepthCallbackMin = 0;
  int BinaryMetadataFeatures = 0;
  int OverflowPatternExclusions = 0;
  int MsanTrackOrigins = 0;
  bool MsanUseAfterDtor = true;
  bool MsanParamRetval = true;
  bool CfiCrossDso = false;
  bool CfiICallGeneralizePointers = false;
  bool CfiICallNormalizeIntegers = false;
  bool CfiCanonicalJumpTables = false;
  bool KcfiArity = false;
  std::optional<std::string> KcfiHash;
  int AsanFieldPadding = 0;
  bool SharedRuntime = false;
  bool StableABI = false;
  bool AsanUseAfterScope = true;
  bool AsanPoisonCustomArrayCookie = false;
  bool AsanGlobalsDeadStripping = false;
  bool AsanUseOdrIndicator = false;
  bool AsanInvalidPointerCmp = false;
  bool AsanInvalidPointerSub = false;
  bool AsanOutlineInstrumentation = false;
  llvm::AsanDtorKind AsanDtorKind = llvm::AsanDtorKind::Invalid;
  std::string HwasanAbi;
  bool LinkRuntimes = true;
  bool LinkCXXRuntimes = false;
  bool NeedPIE = false;
  bool SafeStackRuntime = false;
  bool Stats = false;
  bool TsanMemoryAccess = true;
  bool TsanFuncEntryExit = true;
  bool TsanAtomics = true;
  bool MinimalRuntime = false;
  bool TrapLoop = false;
  bool TysanOutlineInstrumentation = true;
  bool HandlerPreserveAllRegs = false;
  // True if cross-dso CFI support if provided by the system (i.e. Android).
  bool ImplicitCfiRuntime = false;
  bool NeedsMemProfRt = false;
  bool HwasanUseAliases = false;
  llvm::AsanDetectStackUseAfterReturnMode AsanUseAfterReturn =
      llvm::AsanDetectStackUseAfterReturnMode::Invalid;

  std::string MemtagMode;
  bool AllocTokenFastABI = false;
  bool AllocTokenExtended = false;

  /// P3100: parse every -fsanitize-semantic= argument, validate each request,
  /// and record accepted per-bit semantics into ExplicitSemantics.
  void parseSanitizeSemanticArgs(const Driver &TCDriver,
                                 const llvm::opt::ArgList &Args,
                                 bool DiagnoseErrors);

  /// P3100: record an accepted explicit per-bit semantic (later overrides win).
  void storeExplicitSemantic(SanitizerMask Bit,
                             ContractEvaluationSemantic Semantic);

  /// P3100 Task 4.1: without -fcontracts-p4298, a routed check that resolves to
  /// a noexcept_* semantic -- explicit or derived from -fsanitize-recover= --
  /// is a hard error (a throwing handler cannot propagate).  Applied after all
  /// masks and explicit semantics are known, so the derived case is covered.
  void applyRoutedSemanticP4298Gate(const Driver &TCDriver,
                                    bool DiagnoseErrors);

public:
  /// Parses the sanitizer arguments from an argument list.
  SanitizerArgs(const ToolChain &TC, const llvm::opt::ArgList &Args,
                bool DiagnoseErrors = true, bool DiagnoseBoundArchErrors = true,
                BoundArch BA = {},
                Action::OffloadKind DeviceOffloadKind = Action::OFK_None);

  bool needsSharedRt() const { return SharedRuntime; }
  bool needsStableAbi() const { return StableABI; }

  bool needsMemProfRt() const { return NeedsMemProfRt; }
  bool needsAsanRt() const { return Sanitizers.has(SanitizerKind::Address); }
  bool needsHwasanRt() const {
    return Sanitizers.has(SanitizerKind::HWAddress);
  }
  bool needsHwasanAliasesRt() const {
    return needsHwasanRt() && HwasanUseAliases;
  }
  bool needsTysanRt() const { return Sanitizers.has(SanitizerKind::Type); }
  bool needsTsanRt() const { return Sanitizers.has(SanitizerKind::Thread); }
  bool needsMsanRt() const { return Sanitizers.has(SanitizerKind::Memory); }
  bool needsFuzzer() const { return Sanitizers.has(SanitizerKind::Fuzzer); }
  bool needsLsanRt() const {
    return Sanitizers.has(SanitizerKind::Leak) &&
           !Sanitizers.has(SanitizerKind::Address) &&
           !Sanitizers.has(SanitizerKind::HWAddress);
  }
  bool needsFuzzerInterceptors() const;
  bool needsUbsanRt() const;
  bool needsUbsanCXXRt() const;
  bool requiresMinimalRuntime() const { return MinimalRuntime; }
  bool needsUbsanLoopDetectRt() const { return TrapLoop; }
  bool needsDfsanRt() const { return Sanitizers.has(SanitizerKind::DataFlow); }
  bool needsSafeStackRt() const { return SafeStackRuntime; }
  bool needsCfiCrossDsoRt() const;
  bool needsCfiCrossDsoDiagRt() const;
  bool needsStatsRt() const { return Stats; }
  bool needsScudoRt() const { return Sanitizers.has(SanitizerKind::Scudo); }
  bool needsNsanRt() const {
    return Sanitizers.has(SanitizerKind::NumericalStability);
  }
  bool needsRtsanRt() const { return Sanitizers.has(SanitizerKind::Realtime); }

  bool hasMemTag() const {
    return hasMemtagHeap() || hasMemtagStack() || hasMemtagGlobals();
  }
  bool hasMemtagHeap() const {
    return Sanitizers.has(SanitizerKind::MemtagHeap);
  }
  bool hasMemtagStack() const {
    return Sanitizers.has(SanitizerKind::MemtagStack);
  }
  bool hasMemtagGlobals() const {
    return Sanitizers.has(SanitizerKind::MemtagGlobals);
  }
  const std::string &getMemtagMode() const {
    assert(!MemtagMode.empty());
    return MemtagMode;
  }

  bool hasShadowCallStack() const {
    return Sanitizers.has(SanitizerKind::ShadowCallStack);
  }

  /// P3100 Task 4.1: TRUE iff a check whose flag mask includes Bit is ROUTED
  /// to the C++ contract-violation handler at run time (today only the address
  /// check).  A routed check's handler runs inside libasan's implicitly-noexcept
  /// error-report path, so a throwing handler can never propagate; its allowed
  /// set is the non-throwing one (assume / quick_enforce / the D4298
  /// noexcept_* semantics).
  static bool isRoutedSanitizerCheck(SanitizerMask Bit);

  /// P3100 Task 1.1: the contract evaluation semantic explicitly requested for
  /// the single-bit check Bit via -fsanitize-semantic=, or Ignore (used as the
  /// "no explicit request" sentinel; Ignore itself is never a legal stored
  /// value) if none.  Returns true and sets Out when an explicit entry exists.
  bool explicitSanitizerSemantic(SanitizerMask Bit,
                                 ContractEvaluationSemantic &Out) const;

  /// P3100 Task 1.2/4.1: the final resolved contract evaluation semantic for
  /// the single-bit check Bit -- the explicit -fsanitize-semantic= value if
  /// set, else derived from -fsanitize/-fsanitize-recover/-fsanitize-trap
  /// (routed checks use the p4298-gated non-throwing derivation).  This is the
  /// value CodeGen (CL2) consumes; the driver re-renders it to cc1.
  ContractEvaluationSemantic resolvedSanitizerSemantic(SanitizerMask Bit) const;

  bool requiresPIE() const;
  bool needsUnwindTables() const;
  bool needsLTO() const;
  bool linkRuntimes() const { return LinkRuntimes; }
  bool linkCXXRuntimes() const { return LinkCXXRuntimes; }
  bool hasCrossDsoCfi() const { return CfiCrossDso; }
  bool hasAnySanitizer() const { return !Sanitizers.empty(); }
  void addArgs(const ToolChain &TC, const llvm::opt::ArgList &Args,
               llvm::opt::ArgStringList &CmdArgs, types::ID InputType) const;
};

}  // namespace driver
}  // namespace clang

#endif
