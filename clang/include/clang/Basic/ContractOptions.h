//===- ContractOptions.h - C++ Contract Options -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// Defines common enums and types used by contracts and contract attributes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_CONTRACT_OPTIONS_H
#define LLVM_CLANG_BASIC_CONTRACT_OPTIONS_H

#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace llvm {
namespace vfs {
class FileSystem;
} // namespace vfs
} // namespace llvm

namespace clang {
using llvm::StringRef;
class ASTContext;
class DeclContext;
class DiagnosticsEngine;
class SourceManager;

enum class ContractTag { No = 0, Yes = 1 };

constexpr ContractTag NotAContract = ContractTag::No;
constexpr ContractTag IsAContract = ContractTag::Yes;
constexpr ContractTag InContract = ContractTag::Yes;
constexpr ContractTag NotInContract = ContractTag::No;

enum class ContractGroupDiagnostic {
  // The remaining values map to %select values in diagnostics in both
  // DiagnosticSemaKinds.td and DiagnosticDriverKinds.td.
  InvalidFirstChar,
  InvalidChar,
  InvalidLastChar,
  EmptySubGroup,
  Empty,
  InvalidSemantic,
};

/// The kind of contract.
///
/// NOTE: These enumerations _do not_ match the values in
/// std::contracts::assertion_kind because they start at 1.
enum class ContractKind {
  /// A function precondition
  Pre,

  /// A function post condition
  Post,

  /// contract_assert
  Assert,

  /// P3100 implicit contract assertion guarding core-language UB
  Implicit
};

/// std::contracts::assertion_kind
enum class ContractAssertionKind {
  Pre = 1,

  Post = 2,

  Assert = 3,

  Manual = 4,

  CAssert = 5
};

/// Contract evaluation mode. Determines whether to check contracts, and
/// whether contract failures cause compile errors.
///
/// Values match std::contracts::evaluation_semantic per [support.contract.enum].
enum class ContractEvaluationSemantic {
  Ignore = 1,
  Observe = 2,
  Enforce = 3,
  QuickEnforce = 4,
  Assume = 5, // P3100 "assume" evaluation semantic
  NoexceptObserve = 6, // D4298
  NoexceptEnforce = 7, // D4298
};

/// Bitmask (bit N == ContractEvaluationSemantic value N) of the standard
/// evaluation semantics.  This is the default "all allowed" set for a
/// contract's allowed-semantics mask.  Note that QuickEnforce (bit 4) IS
/// included: the previous default of 0xF silently excluded it (and set the
/// unused bit 0), which meant a contract that resolved to quick_enforce was
/// downgraded through the fallback chain.
inline constexpr unsigned AllContractSemanticsMask =
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Observe)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Enforce)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::QuickEnforce));

/// Every valid evaluation semantic, including P3100 "assume".  This is the
/// flag-independent "no restriction" set that a label's allowed_semantics
/// facet narrows; the -fcontracts-allow-assume gate is applied separately.
inline constexpr unsigned AllContractSemanticsMaskWithAssume =
    AllContractSemanticsMask |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Assume));

/// Every valid evaluation semantic, including "assume" and the D4298
/// noexcept-terminating variants.  This is the flag-independent "no
/// restriction" set that a label's allowed_semantics facet narrows; the
/// -fcontracts-allow-assume / -fcontracts-p4298 gates are applied
/// separately.
inline constexpr unsigned AllContractSemanticsMaskWithExtensions =
    AllContractSemanticsMaskWithAssume |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::NoexceptEnforce)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::NoexceptObserve));

/// The gated base set: the four standard semantics, plus "assume" only when
/// -fcontracts-allow-assume is in effect.  The (flag-independent) label
/// restriction is intersected with this at query construction, so "assume"
/// can never be present when the flag is off -- a label cannot re-add it.
inline constexpr unsigned gatedContractSemanticsMask(bool AllowAssume,
                                                      bool EnableP4298) {
  unsigned Mask = AllowAssume ? AllContractSemanticsMaskWithAssume
                              : AllContractSemanticsMask;
  if (EnableP4298)
    Mask |= (1u << static_cast<unsigned>(ContractEvaluationSemantic::NoexceptEnforce)) |
            (1u << static_cast<unsigned>(ContractEvaluationSemantic::NoexceptObserve));
  return Mask;
}

/// Parse a contract evaluation semantic name (e.g. "observe",
/// "noexcept_enforce") to its enum value.  Returns false if NAME does not
/// name a known semantic.  Single source of truth for semantic-name spelling,
/// shared by -fcontracts-group-evaluation-semantic= parsing and the P3100
/// -fsanitize-semantic= driver parsing.
inline bool contractSemanticFromName(StringRef Name,
                                     ContractEvaluationSemantic &Out) {
  if (Name == "ignore")
    Out = ContractEvaluationSemantic::Ignore;
  else if (Name == "observe")
    Out = ContractEvaluationSemantic::Observe;
  else if (Name == "enforce")
    Out = ContractEvaluationSemantic::Enforce;
  else if (Name == "quick_enforce")
    Out = ContractEvaluationSemantic::QuickEnforce;
  else if (Name == "assume")
    Out = ContractEvaluationSemantic::Assume;
  else if (Name == "noexcept_enforce")
    Out = ContractEvaluationSemantic::NoexceptEnforce;
  else if (Name == "noexcept_observe")
    Out = ContractEvaluationSemantic::NoexceptObserve;
  else
    return false;
  return true;
}

/// The canonical spelling for a contract evaluation semantic value; the
/// inverse of contractSemanticFromName.  Returns "invalid" for an
/// out-of-range value.
inline StringRef contractSemanticName(ContractEvaluationSemantic Semantic) {
  switch (Semantic) {
  case ContractEvaluationSemantic::Ignore:
    return "ignore";
  case ContractEvaluationSemantic::Observe:
    return "observe";
  case ContractEvaluationSemantic::Enforce:
    return "enforce";
  case ContractEvaluationSemantic::QuickEnforce:
    return "quick_enforce";
  case ContractEvaluationSemantic::Assume:
    return "assume";
  case ContractEvaluationSemantic::NoexceptEnforce:
    return "noexcept_enforce";
  case ContractEvaluationSemantic::NoexceptObserve:
    return "noexcept_observe";
  }
  return "invalid";
}

// Values match std::contracts::detection_mode per [support.contract.enum].
enum class ContractDetectionMode {
  PredicateFailed = 1,
  ExceptionRaised = 2,
  Unspecified = 3
};

/// The code generation style for the contract. Inline
enum class ContractEmissionStyle {
  /// Emit the contract at every call site, in the caller.
  InCaller,

  /// Emit the contract with the function definition.
  InDefinition,
};

/// Indicates whether the contract-scope information was pushed before the
/// function's declaration context was available (and so needs special handling
/// when adjusting the context).
enum ContractScopeOffset {
  CSO_ParentContext,
  CSO_FunctionContext
};

/// Source type for a contract configuration entry.
enum class ContractConfigSourceKind {
  GroupSemantic,
  JSONInline,
  JSONFile,
};

/// The parsed P3595 configuration storage.  Its layout (in particular
/// ContractConfigEntry) evolves as new match criteria are added, so it is
/// defined in the impl-only header clang/Basic/ContractConfig.h and held by
/// ContractOptions behind a pointer -- keeping it out of the include graph of
/// everything that pulls in LangOptions.h.
struct ContractConfigData;
struct ContractConfigSource;

/// P3595 dynamic (link-/run-time) selection: the "output.dynamic" descriptor
/// of a matched contract config entry.  Companion to
/// ContractOptions::resolveContractSemantic (see resolveContractDynamic).
/// `Found` is false when the matched entry has no "dynamic" descriptor, or
/// when resolving in a constant-evaluation context -- a dynamic selector is
/// never invoked at compile time, so no descriptor is surfaced there (the
/// entry's Semantic is used instead; see the constant-evaluation rule in
/// resolveContractSemantic's implementation).
struct ContractDynamicResult {
  std::string Name;
  int Linkage = 0;       // 0 = "C++", 1 = "C"; meaningful only when Found
  bool ProvideWeak = false;
  bool Found = false;
};

/// Per-assertion query for P3595 contract config resolution.
/// Namespace and location are lazily computed from the DeclContext
/// and SourceLocation to avoid cost when no config entry needs them.
struct ContractQuery {
  ContractKind Kind;
  bool CallerSide = false;
  bool InConstantEvaluation = false;
  unsigned AllowedMask = AllContractSemanticsMask;
  llvm::ArrayRef<std::string> Groups;

  const DeclContext *FnContext = nullptr;
  SourceLocation Loc;
  const SourceManager *SM = nullptr;

  llvm::StringRef getNamespace() const;
  llvm::StringRef getLocationFile() const;
  int getLocationLine() const;

private:
  mutable std::string CachedNamespace;
  mutable std::string CachedLocationFile;
  mutable int CachedLocationLine = 0;
  mutable bool NamespaceComputed = false;
  mutable bool LocationComputed = false;
};

/// Represents the set of contract groups that have been enabled or disabled
/// on the command line using '-fclang-contract-groups='.
///
/// A contract group is a string consisting of identifiers join by '.'. For
/// example, "a.b.c" is a contract group with three subgroups.
///
/// When determining if a particular contract check is enabled, we check if
/// the user has enabled/disabled the group/subgroup that the check belongs to,
/// in order of specificity. For example, passing
///   '-fclang-contract-groups=-std,+std.hardening,-std.hardening.foo'
/// will enable 'std.hardening.baz', but disable 'std.hardening.foo.bar' and
/// 'std.baz'.
///
/// TODO: Should we match in the same manner as clang-tidy checks?
///    Specifically, allow the use of '*' and drop all notion of groups?
class ContractOptions {
public:
  ContractOptions() = default;

  using DiagnoseGroupFunc =
      std::function<void(ContractGroupDiagnostic, StringRef, StringRef)>;

  static bool validateContractGroup(llvm::StringRef GroupAndValue,
                                    const DiagnoseGroupFunc &Diagnoser);

  /// Validate a -fcontracts-group-evaluation-semantic= argument, reporting
  /// problems through Diagnoser.  A bare "semantic" (no group) sets
  /// DefaultSemantic; a "group:semantic" entry is validated here and recorded
  /// as a configuration source (see addConfigSource) rather than stored
  /// separately.
  void addUnparsedContractGroup(StringRef GroupAndValue,
                                const DiagnoseGroupFunc &Diagnoser);

  /// The default semantics for contracts (the catch-all used by initConfig).
  /// Set by -fcontract-evaluation-semantic= (marshalled) or a bare
  /// -fcontracts-group-evaluation-semantic=<semantic>.
  ContractEvaluationSemantic DefaultSemantic =
      ContractEvaluationSemantic::Enforce;

  /// P3100: whether the "assume" evaluation semantic may actually be used.
  /// Set by -fcontracts-allow-assume.  When false, the query builders omit
  /// the assume bit from a contract's allowed set, so a resolved "assume" is
  /// adjusted to "ignore" (see resolveContractSemantic).
  bool AllowAssume = false;

  /// P3595 configuration: resolve the evaluation semantic for a contract.
  ContractEvaluationSemantic
  resolveContractSemantic(const ContractQuery &Query) const;

  /// Clamp a candidate evaluation semantic into an allowed set: apply the
  /// P3100 "assume selected but disallowed -> ignore" rule, then the generic
  /// fallback order ({observe, enforce, quick_enforce, ignore}).  Shared by
  /// resolveContractSemantic and the P3595 dynamic transform-table
  /// precomputation so both clamp identically.  Static: it needs no config.
  static ContractEvaluationSemantic
  clampToAllowed(ContractEvaluationSemantic Candidate, unsigned AllowedMask);

  /// P3595 dynamic selection: resolve the "output.dynamic" descriptor for a
  /// contract, running the same match scan (and the same
  /// constant-evaluation skip rule) as resolveContractSemantic.  This is a
  /// companion query rather than a change to resolveContractSemantic's
  /// return type so its existing call sites need not change; a caller that
  /// also needs the dynamic descriptor calls both.
  ContractDynamicResult
  resolveContractDynamic(const ContractQuery &Query) const;

  /// Add a configuration source in command-line order.
  void addConfigSource(ContractConfigSourceKind Kind, llvm::StringRef Arg);

  /// Parse JSON text and append config entries. SourceDesc is for diagnostics,
  /// which are reported through Diags.
  void parseConfigJSON(DiagnosticsEngine &Diags, llvm::StringRef JSON,
                       llvm::StringRef SourceDesc);

  /// Read a JSON file (through \p VFS) and parse it as contract configuration.
  void parseConfigFile(DiagnosticsEngine &Diags, llvm::vfs::FileSystem &VFS,
                       llvm::StringRef Path);

  /// The configuration sources recorded in command-line order.  Empty when
  /// no configuration has been supplied.
  llvm::ArrayRef<ContractConfigSource> getConfigSources() const;

  /// Build the resolved entry list from the configuration sources, reporting
  /// any problems through Diags (when non-null) and reading any configuration
  /// files through VFS (required when a JSON-file source is present).  Runs
  /// once; subsequent calls are a no-op.  Called with Diags and VFS from
  /// CompilerInstance (before semantic analysis) and lazily (without) from
  /// resolveContractSemantic, which by then finds it already initialized.
  void initConfig(DiagnosticsEngine *Diags = nullptr,
                  llvm::vfs::FileSystem *VFS = nullptr) const;

  /// True iff the effective configuration contains an entry that requests
  /// caller-side contract checking with a semantic other than "ignore".
  /// Clang does not yet emit caller-side checks (see P3595); callers of
  /// this predicate use it to warn that such configuration is a no-op.
  bool configRequestsCallerSideChecks() const;

private:
  /// Lazily-allocated parsed configuration (see ContractConfigData).  Shared
  /// so that copies of ContractOptions (LangOptions is copied) share the
  /// post-init, effectively-immutable configuration rather than deep-copying
  /// it.  Null until the first configuration source is added.
  mutable std::shared_ptr<ContractConfigData> Config;

  /// Ensure Config is allocated; returns it.
  ContractConfigData &ensureConfig() const;
};

} // namespace clang

#endif // LLVM_CLANG_BASIC_CONTRACT_OPTIONS_H
